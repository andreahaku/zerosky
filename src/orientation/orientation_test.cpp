/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "calibration.h"
#include "magnetic_gate.h"
#include "mahony.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void close_to(double actual, double expected, double tolerance = 1e-4) {
    require(std::abs(actual - expected) <= tolerance,
            "Orientation result " + std::to_string(actual) + " differs from " +
                std::to_string(expected));
}

std::string profile_json(const std::string& soft_iron =
                         "[[2,0,0],[0,1,0],[0,0,1]]") {
    return "{\"schema\":1,\"units\":\"uT\",\"sensor\":\"bmm150\","
           "\"sensor_to_body\":[[1,0,0],[0,1,0],[0,0,1]],"
           "\"profiles\":{\"none\":{\"offset_uT\":[1,2,3],\"soft_iron\":" +
           soft_iron +
           ",\"field_uT\":44,\"rms_uT\":0.8,\"samples\":800},"
           "\"cap\":null,\"usb_gps\":null}}";
}

} // namespace

int main() {
    using namespace zerosky::orientation;
    using zerosky::sensors::Vec3;
    namespace fs = std::filesystem;

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = fs::temp_directory_path() /
                      ("zerosky-orientation-test-" + std::to_string(stamp) + ".json");
    auto save = [&](const std::string& json) {
        std::ofstream file(path, std::ios::trunc);
        require(static_cast<bool>(file), "Cannot create test calibration");
        file << json;
        require(file.good(), "Cannot write test calibration");
    };
    save(profile_json());
    Calibration calibration;
    std::string error;
    require(load_calibration(path, "none", calibration, error), error);
    require(calibration.configuration == "none", "Wrong profile chosen");
    require(calibration.samples == 800, "Sample count lost");
    const Vec3 corrected = calibrate_magnetometer(calibration, {2, 4, 6});
    close_to(corrected.x, 2.0);
    close_to(corrected.y, 2.0);
    close_to(corrected.z, 3.0);
    require(!load_calibration(path, "usb_gps", calibration, error),
            "Missing USB profile accepted");
    require(calibration.configuration == "none", "Failed load changed active profile");
    save(profile_json("[[1,0,0],[0,-1,0],[0,0,1]]"));
    require(!load_calibration(path, "none", calibration, error),
            "Non-positive soft-iron matrix accepted");
    save("{\"schema\":1,\"profiles\":{}");
    require(!load_calibration(path, "none", calibration, error),
            "Malformed calibration JSON accepted");
    fs::remove(path);

    Mahony filter;
    constexpr double angle = 0.6;
    const Quaternion truth{std::cos(angle / 2), 0, 0, std::sin(angle / 2)};
    const Vec3 world_field{8.0, 35.0, -20.0};
    const Vec3 body_field = Mahony::rotate_inverse(truth, world_field);
    const Vec3 body_up = Mahony::rotate_inverse(truth, {0, 0, 9.80665});
    for (int i = 0; i < 3000; ++i) {
        require(filter.update({0, 0, 0}, body_up, body_field, world_field, 0.01),
                "Mahony rejected valid sample");
    }
    require(filter.used_magnetometer(), "Magnetic correction unused");
    const Vec3 world_east = Mahony::rotate(filter.body_to_enu(), {1, 0, 0});
    close_to(world_east.x, std::cos(angle), 0.01);
    close_to(world_east.y, std::sin(angle), 0.01);
    require(!filter.update({0, 0, 0}, body_up, {}, {}, -0.01),
            "Invalid time step accepted");

    filter.reset();
    for (int i = 0; i < 100; ++i)
        require(filter.update({0, 0, 1}, {0, 0, 9.80665}, {}, {}, 0.01),
                "Six-axis update failed");
    require(!filter.used_magnetometer(), "Six-axis mode used magnetometer");
    const Vec3 spun_east = Mahony::rotate(filter.body_to_enu(), {1, 0, 0});
    close_to(std::atan2(spun_east.y, spun_east.x), 1.0, 0.01);

    MagneticGate gate;
    using Status = MagneticGate::Status;
    const auto t0 = std::chrono::steady_clock::now();
    require(gate.update(44.0, 44.0, t0) == Status::Healthy, "Clean field rejected");
    require(gate.update(108.0, 44.0, t0 + std::chrono::milliseconds(100)) ==
                Status::Suspect, "Disturbance not suspected immediately");
    require(!gate.allow_magnetic_correction(), "Suspect field fed into filter");
    require(gate.update(108.0, 44.0, t0 + std::chrono::milliseconds(1200)) ==
                Status::Disturbed, "Persistent disturbance not detected");
    require(gate.update(44.0, 44.0, t0 + std::chrono::seconds(2)) ==
                Status::Disturbed, "Disturbance cleared too quickly");
    require(gate.update(44.0, 44.0, t0 + std::chrono::seconds(6)) ==
                Status::Healthy, "Clean field did not recover");
    require(gate.update(0.0, 44.0, t0 + std::chrono::seconds(7)) ==
                Status::Unknown, "Invalid measurement not unknown");
}
