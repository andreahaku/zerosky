/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "attitude_service.h"
#include "geomagnetic.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace zerosky::orientation {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr auto kPollPeriod = std::chrono::milliseconds(10);
constexpr auto kMagneticMaxAge = std::chrono::milliseconds(250);
constexpr size_t kMaxCalibrationSamples = 6000;

double norm(sensors::Vec3 v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

bool finite(sensors::Vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

double determinant(const Matrix3& a) {
    return a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
           a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
           a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
}

bool rotation(const Matrix3& a) {
    if (!std::isfinite(determinant(a)) || std::abs(determinant(a) - 1.0) > 0.02)
        return false;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            double dot = 0.0;
            for (size_t k = 0; k < 3; ++k) dot += a[k][i] * a[k][j];
            if (!std::isfinite(dot) || std::abs(dot - (i == j ? 1.0 : 0.0)) > 0.02)
                return false;
        }
    }
    return true;
}

sensors::Vec3 multiply(const Matrix3& a, sensors::Vec3 v) {
    return {a[0][0] * v.x + a[0][1] * v.y + a[0][2] * v.z,
            a[1][0] * v.x + a[1][1] * v.y + a[1][2] * v.z,
            a[2][0] * v.x + a[2][1] * v.y + a[2][2] * v.z};
}

double dot(sensors::Vec3 a, sensors::Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Quaternion product(Quaternion a, Quaternion b) {
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

std::optional<Quaternion> initial_attitude(sensors::Vec3 accel,
                                           sensors::Vec3 magnetic_body,
                                           sensors::Vec3 magnetic_enu) {
    constexpr double kGravity = 9.80665;
    const double accel_norm = norm(accel);
    const double body_field_norm = norm(magnetic_body);
    const double world_field_norm = norm(magnetic_enu);
    if (!finite(accel) || !finite(magnetic_body) || !finite(magnetic_enu) ||
        std::abs(accel_norm - kGravity) > 0.1 * kGravity ||
        body_field_norm < 1e-9 || world_field_norm < 1e-9) return std::nullopt;

    const sensors::Vec3 up{accel.x / accel_norm, accel.y / accel_norm,
                           accel.z / accel_norm};
    const sensors::Vec3 body_field{magnetic_body.x / body_field_norm,
                                   magnetic_body.y / body_field_norm,
                                   magnetic_body.z / body_field_norm};
    const sensors::Vec3 world_field{magnetic_enu.x / world_field_norm,
                                    magnetic_enu.y / world_field_norm,
                                    magnetic_enu.z / world_field_norm};
    // A changed inclination can indicate local magnetic interference even
    // when the field magnitude passes the magnetic gate.
    if (std::abs(dot(up, body_field) - world_field.z) > 0.15 ||
        std::hypot(world_field.x, world_field.y) < 0.2) return std::nullopt;

    // Align measured gravity with ENU up, then rotate about ENU up until the
    // horizontal magnetic field matches WMM. This also handles face-down poses.
    Quaternion tilt{1.0 + up.z, up.y, -up.x, 0.0};
    if (tilt.w < 1e-6) tilt = {0.0, 1.0, 0.0, 0.0};
    else {
        const double length = std::sqrt(tilt.w * tilt.w + tilt.x * tilt.x +
                                        tilt.y * tilt.y);
        tilt = {tilt.w / length, tilt.x / length, tilt.y / length, 0.0};
    }
    const sensors::Vec3 leveled = Mahony::rotate(tilt, body_field);
    const double measured_horizontal = std::hypot(leveled.x, leveled.y);
    if (measured_horizontal < 0.2) return std::nullopt;
    const double yaw = std::atan2(leveled.x * world_field.y - leveled.y * world_field.x,
                                  leveled.x * world_field.x + leveled.y * world_field.y);
    return product({std::cos(yaw / 2.0), 0.0, 0.0, std::sin(yaw / 2.0)}, tilt);
}

} // namespace

AttitudeService::AttitudeService(std::unique_ptr<sensors::Source> source)
    : source_(std::move(source)) {
    latest_.simulated = source_ && source_->simulated();
}

AttitudeService::~AttitudeService() { stop(); }

bool AttitudeService::start(std::string& error) {
    error.clear();
    if (!source_) { error = "No sensor source"; return false; }
    if (running_.exchange(true)) { error = "Attitude service already running"; return false; }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        previous_sample_at_ = {};
        last_magnetic_at_ = {};
        heading_good_since_ = {};
        heading_seeded_ = false;
        heading_converged_ = false;
        magnetic_gate_.reset();
        filter_.reset();
        latest_.raw_sample = {};
        latest_.last_raw_magnetic_uT = {};
        latest_.last_raw_magnetic_at = {};
        latest_.compass = CompassStatus::Unreliable;
        latest_.error.clear();
    }
    try {
        worker_ = std::thread(&AttitudeService::run, this);
    } catch (...) {
        running_ = false;
        error = "Cannot start attitude worker";
        return false;
    }
    return true;
}

void AttitudeService::stop() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
    std::lock_guard<std::mutex> lock(mutex_);
    latest_.compass = CompassStatus::Unreliable;
    capturing_ = false;
}

bool AttitudeService::select_configuration(const std::string& hardware,
                                            const std::filesystem::path& file,
                                            std::string& error) {
    Calibration loaded;
    const bool valid = load_calibration(file, hardware, loaded, error);
    std::lock_guard<std::mutex> lock(mutex_);
    hardware_ = hardware;
    calibration_valid_ = valid;
    calibration_ = valid ? std::move(loaded) : Calibration{};
    magnetic_gate_.reset();
    last_magnetic_at_ = {};
    heading_good_since_ = {};
    heading_seeded_ = false;
    heading_converged_ = false;
    latest_.compass = CompassStatus::Unreliable;
    return valid;
}

bool AttitudeService::set_frame_mapping(FrameMapping mapping, std::string& error) {
    error.clear();
    const double pointing_norm = norm(mapping.pointing_body);
    const double screen_up_norm = norm(mapping.screen_up_body);
    if (!rotation(mapping.accel_to_body) || !rotation(mapping.gyro_to_body) ||
        !finite(mapping.pointing_body) || !std::isfinite(pointing_norm) ||
        pointing_norm < 1e-9 || !finite(mapping.screen_up_body) ||
        !std::isfinite(screen_up_norm) || screen_up_norm < 1e-9) {
        error = "Invalid sensor-to-body frame mapping";
        return false;
    }
    mapping.pointing_body = {mapping.pointing_body.x / pointing_norm,
                             mapping.pointing_body.y / pointing_norm,
                             mapping.pointing_body.z / pointing_norm};
    mapping.screen_up_body = {mapping.screen_up_body.x / screen_up_norm,
                              mapping.screen_up_body.y / screen_up_norm,
                              mapping.screen_up_body.z / screen_up_norm};
    const double along = mapping.pointing_body.x * mapping.screen_up_body.x +
                         mapping.pointing_body.y * mapping.screen_up_body.y +
                         mapping.pointing_body.z * mapping.screen_up_body.z;
    if (!std::isfinite(along) || std::abs(along) > 0.999) {
        error = "Pointing and screen-up axes are nearly parallel";
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    mapping_ = mapping;
    filter_.reset();
    previous_sample_at_ = {};
    heading_good_since_ = {};
    heading_seeded_ = false;
    heading_converged_ = false;
    latest_.compass = CompassStatus::Unreliable;
    return true;
}

void AttitudeService::set_magnetic_reference(MagneticReference reference) {
    if (!finite(reference.field_enu_uT) || norm(reference.field_enu_uT) < 1e-9)
        reference.valid = false;
    std::lock_guard<std::mutex> lock(mutex_);
    const sensors::Vec3 change{reference.field_enu_uT.x - reference_.field_enu_uT.x,
                                reference.field_enu_uT.y - reference_.field_enu_uT.y,
                                reference.field_enu_uT.z - reference_.field_enu_uT.z};
    const bool validity_changed = reference.valid != reference_.valid ||
                                  norm(change) > 0.5;
    reference_ = reference;
    if (validity_changed) {
        magnetic_gate_.reset();
        last_magnetic_at_ = {};
        heading_good_since_ = {};
        heading_seeded_ = false;
        heading_converged_ = false;
        latest_.compass = CompassStatus::Unreliable;
    }
}

bool AttitudeService::set_geomagnetic_location(const GeomagneticModel& model,
                                               double latitude_deg, double longitude_deg,
                                               double height_wgs84_m, double decimal_year,
                                               std::string& error) {
    const auto field = model.field(latitude_deg, longitude_deg,
                                   height_wgs84_m, decimal_year);
    if (!field) {
        set_magnetic_reference({});
        error = "No valid WMM2025 reference for position and date";
        return false;
    }
    set_magnetic_reference({field->enu_uT, true});
    error.clear();
    return true;
}

AttitudeSnapshot AttitudeService::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_;
}

bool AttitudeService::begin_calibration_capture(const std::string& hardware,
                                                std::string& error) {
    error.clear();
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_ || hardware.empty() || hardware != hardware_) {
        error = "Calibration capture requires the active hardware configuration";
        return false;
    }
    captured_.clear();
    captured_.reserve(kMaxCalibrationSamples);
    capturing_ = true;
    return true;
}

std::vector<CalibrationSample> AttitudeService::end_calibration_capture() {
    std::lock_guard<std::mutex> lock(mutex_);
    capturing_ = false;
    return std::exchange(captured_, {});
}

void AttitudeService::run() {
    auto next = std::chrono::steady_clock::now();
    while (running_) {
        sensors::Sample sample;
        std::string error;
        if (source_->read(sample, error)) {
            process(sample, error);
        } else {
            std::lock_guard<std::mutex> lock(mutex_);
            latest_.compass = CompassStatus::Unreliable;
            latest_.error = std::move(error);
        }
        next += kPollPeriod;
        const auto now = std::chrono::steady_clock::now();
        if (next < now) next = now + kPollPeriod;
        std::this_thread::sleep_until(next);
    }
}

void AttitudeService::process(const sensors::Sample& sample,
                              const std::string& source_error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (sample.sampled_at == std::chrono::steady_clock::time_point{} ||
        (previous_sample_at_ != std::chrono::steady_clock::time_point{} &&
         sample.sampled_at <= previous_sample_at_)) {
        latest_.compass = CompassStatus::Unreliable;
        latest_.error = "Sensor timestamps are not increasing";
        return;
    }
    double dt = previous_sample_at_ == std::chrono::steady_clock::time_point{}
                    ? 0.01
                    : std::chrono::duration<double>(sample.sampled_at - previous_sample_at_).count();
    previous_sample_at_ = sample.sampled_at;
    latest_.raw_sample = sample;
    if (sample.magnetic_present && sample.magnetic_fresh) {
        latest_.last_raw_magnetic_uT = sample.magnetic_uT;
        latest_.last_raw_magnetic_at = sample.sampled_at;
    }
    if (dt > 0.25) {
        filter_.reset(filter_.body_to_enu());
        dt = 0.01;
    }

    if (capturing_ && sample.magnetic_present && sample.magnetic_fresh &&
        captured_.size() < kMaxCalibrationSamples)
        captured_.push_back({sample.sampled_at, sample.magnetic_uT});

    const sensors::Vec3 accel = multiply(mapping_.accel_to_body, sample.acceleration_mps2);
    const sensors::Vec3 gyro = multiply(mapping_.gyro_to_body, sample.angular_velocity_radps);
    if (last_magnetic_at_ != std::chrono::steady_clock::time_point{} &&
        sample.sampled_at - last_magnetic_at_ > kMagneticMaxAge) {
        magnetic_gate_.reset();
        heading_good_since_ = {};
        heading_converged_ = false;
    }
    std::optional<sensors::Vec3> magnetic;
    if (sample.magnetic_present && sample.magnetic_fresh &&
        calibration_valid_ && reference_.valid) {
        const sensors::Vec3 body = calibrate_magnetometer(calibration_, sample.magnetic_uT);
        const double measured = norm(body);
        const double expected = norm(reference_.field_enu_uT);
        magnetic_gate_.update(measured, expected, sample.sampled_at);
        last_magnetic_at_ = sample.sampled_at;
        if (magnetic_gate_.allow_magnetic_correction()) magnetic = body;
    }
    if (!heading_seeded_ && mapping_.verified && magnetic) {
        if (const auto initial = initial_attitude(accel, *magnetic, reference_.field_enu_uT)) {
            filter_.reset(*initial);
            heading_seeded_ = true;
            heading_good_since_ = {};
        }
    }
    const bool updated = filter_.update(gyro, accel, magnetic,
                                       reference_.valid ? std::optional<sensors::Vec3>{reference_.field_enu_uT}
                                                        : std::nullopt, dt);
    latest_.sampled_at = sample.sampled_at;
    latest_.body_to_enu = filter_.body_to_enu();
    latest_.accelerometer_used = filter_.used_accelerometer();
    latest_.magnetometer_used = filter_.used_magnetometer();
    latest_.error = updated ? source_error : "Invalid attitude filter input";

    if (updated && magnetic && filter_.used_accelerometer()) {
        const sensors::Vec3 observed_enu = Mahony::rotate(filter_.body_to_enu(), *magnetic);
        const double observed_horizontal = std::hypot(observed_enu.x, observed_enu.y);
        const double reference_horizontal = std::hypot(reference_.field_enu_uT.x,
                                                        reference_.field_enu_uT.y);
        const double azimuth_difference =
            std::atan2(observed_enu.x, observed_enu.y) -
            std::atan2(reference_.field_enu_uT.x, reference_.field_enu_uT.y);
        const double yaw_error_deg = std::abs(std::atan2(std::sin(azimuth_difference),
                                                         std::cos(azimuth_difference))) *
                                     180.0 / kPi;
        const sensors::Vec3 predicted_up = Mahony::rotate_inverse(
            filter_.body_to_enu(), {0, 0, 1});
        const double accel_norm = norm(accel);
        const double tilt_cosine = accel_norm > 1e-9
                                       ? std::clamp((predicted_up.x * accel.x +
                                                     predicted_up.y * accel.y +
                                                     predicted_up.z * accel.z) / accel_norm,
                                                    -1.0, 1.0)
                                       : -1.0;
        const double tilt_error_deg = std::acos(tilt_cosine) * 180.0 / kPi;
        if (observed_horizontal > 1e-9 && reference_horizontal > 1e-9 &&
            yaw_error_deg <= 5.0 && tilt_error_deg <= 5.0) {
            if (heading_good_since_ == std::chrono::steady_clock::time_point{})
                heading_good_since_ = sample.sampled_at;
            if (sample.sampled_at - heading_good_since_ >= std::chrono::milliseconds(500))
                heading_converged_ = true;
        } else if (yaw_error_deg > 10.0 || tilt_error_deg > 10.0) {
            heading_good_since_ = {};
            heading_converged_ = false;
        }
    }
    if (magnetic_gate_.status() != MagneticGate::Status::Healthy) {
        heading_good_since_ = {};
        heading_converged_ = false;
    }

    const sensors::Vec3 pointing = Mahony::rotate(filter_.body_to_enu(),
                                                   mapping_.pointing_body);
    const double horizontal = std::hypot(pointing.x, pointing.y);
    double azimuth = std::atan2(pointing.x, pointing.y) * 180.0 / kPi;
    if (azimuth < 0.0) azimuth += 360.0;
    latest_.pointing_azimuth_deg = azimuth;
    latest_.pointing_altitude_deg = std::atan2(pointing.z, horizontal) * 180.0 / kPi;

    const bool mag_recent = last_magnetic_at_ != std::chrono::steady_clock::time_point{} &&
                            sample.sampled_at - last_magnetic_at_ <= kMagneticMaxAge;
    if (!updated || !source_error.empty() || !mapping_.verified ||
        !calibration_valid_ || !reference_.valid ||
        !sample.magnetic_present || !mag_recent) {
        latest_.compass = CompassStatus::Unreliable;
    } else if (magnetic_gate_.status() == MagneticGate::Status::Disturbed) {
        latest_.compass = CompassStatus::Disturbed;
    } else if (magnetic_gate_.status() == MagneticGate::Status::Healthy &&
               heading_converged_) {
        latest_.compass = CompassStatus::Ok;
    } else {
        latest_.compass = CompassStatus::Unreliable;
    }
}

} // namespace zerosky::orientation
