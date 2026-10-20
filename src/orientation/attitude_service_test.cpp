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
    fs::remove(file);
}
