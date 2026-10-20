/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "attitude_service.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

class FixedSource final : public zerosky::sensors::Source {
public:
    bool read(zerosky::sensors::Sample& sample, std::string& error) override {
        error.clear();
        sample = {};
        sample.sampled_at = std::chrono::steady_clock::now();
        sample.acceleration_mps2 = {0, 0, 9.80665};
        sample.magnetic_present = true;
        sample.magnetic_fresh = true;
        sample.magnetic_uT = {0, 40, 0};
        return true;
    }
    bool simulated() const override { return true; }
};

class MovingSource final : public zerosky::sensors::Source {
public:
    explicit MovingSource(double roll_rad = 0.7, double yaw_rate_radps = 0.15)
        : roll_rad_(roll_rad), yaw_rate_radps_(yaw_rate_radps) {}

    bool read(zerosky::sensors::Sample& sample, std::string& error) override {
        using zerosky::orientation::Mahony;
        using zerosky::orientation::Quaternion;
        error.clear();
        sample = {};
        sample.sampled_at = std::chrono::steady_clock::now();
        if (started_ == std::chrono::steady_clock::time_point{}) started_ = sample.sampled_at;
        const double elapsed = std::chrono::duration<double>(sample.sampled_at - started_).count();
        const double yaw = 2.0943951023931953 + yaw_rate_radps_ * elapsed;
        const double cy = std::cos(yaw / 2.0);
        const double sy = std::sin(yaw / 2.0);
        const double cr = std::cos(roll_rad_ / 2.0);
        const double sr = std::sin(roll_rad_ / 2.0);
        const Quaternion body_to_enu{cy * cr, cy * sr, sy * sr, sy * cr};
        sample.acceleration_mps2 = Mahony::rotate_inverse(body_to_enu, {0, 0, 9.80665});
        sample.angular_velocity_radps =
            Mahony::rotate_inverse(body_to_enu, {0, 0, yaw_rate_radps_});
        sample.magnetic_present = true;
        sample.magnetic_fresh = true;
        sample.magnetic_uT = Mahony::rotate_inverse(body_to_enu, {8, 35, -20});
        return true;
    }
    bool simulated() const override { return true; }

private:
    std::chrono::steady_clock::time_point started_{};
    double roll_rad_;
    double yaw_rate_radps_;
};

} // namespace

int main() {
    using namespace zerosky::orientation;
    namespace fs = std::filesystem;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto file = fs::temp_directory_path() /
                      ("zerosky-attitude-test-" + std::to_string(stamp) + ".json");
    {
        std::ofstream out(file);
        require(static_cast<bool>(out), "Cannot create calibration fixture");
        out << R"({"schema":1,"units":"uT","sensor":"bmm150",)"
               R"("sensor_to_body":[[1,0,0],[0,1,0],[0,0,1]],)"
               R"("profiles":{"none":{"offset_uT":[0,0,0],)"
               R"("soft_iron":[[1,0,0],[0,1,0],[0,0,1]],)"
               R"("field_uT":40,"rms_uT":0.2,"samples":800}}})";
        require(out.good(), "Cannot write calibration fixture");
    }

    AttitudeService service(std::make_unique<FixedSource>());
    std::string error;
    require(!service.select_configuration("usb_gps", file, error),
            "Missing profile accepted");
    require(service.snapshot().compass == CompassStatus::Unreliable,
            "Missing profile did not make compass unreliable");
    require(service.select_configuration("none", file, error), error);
    FrameMapping mapping;
    mapping.pointing_body = {0, 1, 0};
    require(!service.set_frame_mapping(mapping, error),
            "parallel screen-up axis accepted");
    mapping.screen_up_body = {0, 0, 1};
    require(service.set_frame_mapping(mapping, error), error);
    service.set_magnetic_reference({{0, 40, 0}, true});
    require(service.start(error), error);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    require(service.snapshot().compass == CompassStatus::Unreliable,
            "Unverified sensor mounting reported a reliable heading");
    mapping.verified = true;
    require(service.set_frame_mapping(mapping, error), error);
    require(service.begin_calibration_capture("none", error), error);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    AttitudeSnapshot current;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        current = service.snapshot();
    } while (current.compass != CompassStatus::Ok &&
             std::chrono::steady_clock::now() < deadline);
    require(current.compass == CompassStatus::Ok, current.error);
    require(current.simulated, "Simulated source not labeled");
    require(std::abs(current.pointing_azimuth_deg) < 0.01, "Wrong azimuth");
    require(std::abs(current.pointing_altitude_deg) < 0.01, "Wrong altitude");
    const auto captured = service.end_calibration_capture();
    require(!captured.empty(), "No raw magnetic samples captured");
    require(captured.size() <= 6000, "Capture exceeded its bound");
    service.stop();

    // Starting more than 120 degrees from the true heading used to take many
    // seconds at the normal magnetic gain. The first valid field now seeds it.
    AttitudeService moving(std::make_unique<MovingSource>());
    require(moving.select_configuration("none", file, error), error);
    require(moving.set_frame_mapping(mapping, error), error);
    moving.set_magnetic_reference({{8, 35, -20}, true});
    const auto begun = std::chrono::steady_clock::now();
    require(moving.start(error), error);
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    require(std::abs(moving.snapshot().body_to_enu.z) > 0.7,
            "Initial magnetic field did not seed the heading");
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        current = moving.snapshot();
    } while (current.compass != CompassStatus::Ok &&
             std::chrono::steady_clock::now() - begun < std::chrono::seconds(1));
    require(current.compass == CompassStatus::Ok,
            "Slowly moving source did not converge within one second");
    const auto moving_field = Mahony::rotate(current.body_to_enu,
                                             current.raw_sample.magnetic_uT);
    require(std::hypot(moving_field.x - 8.0, moving_field.y - 35.0,
                       moving_field.z + 20.0) < 1.0,
            "Seeded moving heading differs from the reference field");
    moving.stop();

    // The gravity alignment has an explicit antiparallel case for face-down
    // use, which is common during calibration and when aiming the back up.
    AttitudeService inverted(std::make_unique<MovingSource>(3.141592653589793, 0.0));
    require(inverted.select_configuration("none", file, error), error);
    require(inverted.set_frame_mapping(mapping, error), error);
    inverted.set_magnetic_reference({{8, 35, -20}, true});
    require(inverted.start(error), error);
    const auto inverted_deadline = std::chrono::steady_clock::now() +
                                   std::chrono::seconds(1);
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        current = inverted.snapshot();
    } while (current.compass != CompassStatus::Ok &&
             std::chrono::steady_clock::now() < inverted_deadline);
    require(current.compass == CompassStatus::Ok,
            "Face-down source did not converge within one second");
    require(Mahony::rotate(current.body_to_enu, {0, 0, 1}).z < -0.99,
            "Face-down gravity was not aligned");
    const auto inverted_field = Mahony::rotate(current.body_to_enu,
                                               current.raw_sample.magnetic_uT);
    require(std::hypot(inverted_field.x - 8.0, inverted_field.y - 35.0,
                       inverted_field.z + 20.0) < 1.0,
            "Seeded face-down heading differs from the reference field");
    inverted.stop();
    fs::remove(file);
}
