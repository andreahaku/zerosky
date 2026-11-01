/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "attitude_controller.h"

#include "attitude_service.h"
#include "camera_from_attitude.h"
#include "frame_mapping_store.h"
#include "geomagnetic.h"
#include "persisted_state.h"
#include "sensor_source.h"

#include <ctime>

namespace zerosky {
namespace {

double decimal_year_now() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    gmtime_r(&now, &utc);
    return orientation::decimal_year_utc(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday)
        .value_or(0.0);
}

std::string utc_timestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    if (!gmtime_r(&now, &utc)) return {};
    char text[32]{};
    if (std::strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0) return {};
    return text;
}

std::filesystem::path calibration_file() {
    return toolkit::config_file("zerosky", "magnetometer-calibration.json");
}

} // namespace

AttitudeController::AttitudeController() : mapping_(std::make_unique<orientation::FrameMapping>()) {
#if USE_DESKTOP
    unavailable_ = "manual";
#endif
}

AttitudeController::~AttitudeController() {
    if (service_) service_->stop();
}

void AttitudeController::start(const sky::Observer& observer, const std::string& hardware,
                               const std::filesystem::path& cof_file) {
#if USE_DESKTOP
    (void)observer;
    (void)hardware;
    (void)cof_file;
#else
    std::string error;
    if (!service_) {
        if (!orientation::load_frame_mapping(orientation::default_frame_mapping_file(), *mapping_,
                                             error)) {
            unavailable_ = "axes not measured";
            return;
        }
        auto source = std::make_unique<sensors::IioSource>();
        if (!source->discover(error)) {
            unavailable_ = "no IMU";
            return;
        }
        magnetometer_present_ = source->info().magnetic_present;
        service_ = std::make_unique<orientation::AttitudeService>(std::move(source));
        if (!service_->set_frame_mapping(*mapping_, error)) {
            unavailable_ = "bad axes file";
            service_.reset();
            return;
        }
        // A missing profile leaves the compass unreliable; the status says so.
        service_->select_configuration(
            hardware, calibration_file(), error);
        if (!service_->start(error)) {
            unavailable_ = "IMU did not start";
            service_.reset();
            return;
        }
    }
    // Loaded on any call, not only the first: a failed load is retried.
    if (!model_) model_ = orientation::GeomagneticModel::load(cof_file, error);
    if (model_) {
        service_->set_geomagnetic_location(*model_, observer.latitude_deg, observer.longitude_deg,
                                           observer.height_m, decimal_year_now(), error);
    }
    unavailable_.clear();
#endif
}

std::optional<sky::Camera> AttitudeController::camera(const sky::Camera& previous) const {
    if (!service_) return std::nullopt;
    return orientation::camera_from_attitude(service_->snapshot(), *mapping_, previous);
}

bool AttitudeController::compass_usable() const {
    // A disturbed compass keeps steering, and the status line warns.
    return service_ && service_->snapshot().compass != orientation::CompassStatus::Unreliable;
}

void AttitudeController::select_hardware(const std::string& hardware) {
    if (!service_) return;
    std::string error;
    service_->select_configuration(
        hardware, calibration_file(), error);
}

bool AttitudeController::begin_calibration(const sky::Observer& observer,
                                           const std::string& hardware, std::string& error) {
    cancel_calibration_capture();
    if (!mapping_->verified) {
        error = "Frame axes are not measured; use zerosky_probe";
        return false;
    }
    if (!service_ || !magnetometer_present_) {
        error = "BMM150 magnetometer is not available";
        return false;
    }
    if (!model_) {
        error = "WMM2025 model is not available";
        return false;
    }
    const auto field = model_->field(observer.latitude_deg, observer.longitude_deg,
                                     observer.height_m, decimal_year_now());
    if (!field) {
        error = "Cannot calculate WMM2025 field for this position and date";
        return false;
    }
    orientation::Matrix3 mag_to_body{};
    if (!orientation::resolve_magnetic_frame(*mapping_, calibration_file(), mag_to_body, error))
        return false;
    const std::string measured_at = utc_timestamp();
    if (measured_at.empty()) { error = "Cannot read the UTC clock"; return false; }
    if (!service_->begin_calibration_capture(hardware, error)) return false;
    calibration_ = std::make_unique<orientation::CalibrationFlow>(
        hardware, field->total_uT, mag_to_body,
        orientation::CalibrationMetadata{measured_at, observer.latitude_deg, observer.longitude_deg},
        calibration_file(), std::chrono::steady_clock::now(), field->inclination_deg);
    return true;
}

AttitudeController::CalibrationStatus AttitudeController::calibration_status() {
    CalibrationStatus out;
    if (!calibration_) { out.error = "Calibration was not started"; return out; }
    const auto now = std::chrono::steady_clock::now();
    if (calibration_->stage() == orientation::CalibrationStage::Capturing) {
        if (calibration_->needs_preview(service_->calibration_capture_count()))
            calibration_->update(service_->calibration_capture_snapshot());
        if (calibration_->should_finish(now))
            calibration_->finish(service_->end_calibration_capture());
    }
    out.stage = calibration_->stage();
    out.coverage = calibration_->coverage();
    out.missing_pose = calibration_->missing_pose();
    out.seconds_left = calibration_->seconds_left(now);
    out.field_uT = calibration_->field_uT();
    out.rms_uT = calibration_->result().rms_uT;
    out.error = calibration_->error();
    return out;
}

bool AttitudeController::save_calibration_capture(std::string& error) {
    if (!calibration_ || !calibration_->save(error)) return false;
    const auto hardware = calibration_->result().configuration;
    if (!service_->select_configuration(hardware, calibration_file(), error)) {
        error = "Profile saved, but reload failed: " + error;
    }
    return true;
}

void AttitudeController::cancel_calibration_capture() {
    if (!calibration_) return;
    if (calibration_->stage() == orientation::CalibrationStage::Capturing && service_)
        service_->end_calibration_capture();
    calibration_->cancel();
    calibration_.reset();
}

std::string AttitudeController::status() const {
    if (!service_) return unavailable_;
    switch (service_->snapshot().compass) {
        case orientation::CompassStatus::Ok: return "";
        case orientation::CompassStatus::Disturbed: return "compass disturbed";
        case orientation::CompassStatus::Unreliable: return "tilt only";
    }
    return "";
}

} // namespace zerosky
