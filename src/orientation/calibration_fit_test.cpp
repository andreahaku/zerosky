/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "calibration_fit.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unistd.h>

using namespace zerosky::orientation;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

std::vector<CalibrationSample> generate(bool hemisphere = false,
                                        bool elongated = false,
                                        bool bad_holdout = false) {
    constexpr double field = 44.9;
    std::mt19937 random(20261020);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    std::normal_distribution<double> noise(0.0, 0.25);
    std::vector<CalibrationSample> samples;
    const Matrix3 transform = elongated
        ? Matrix3{{{5.2, 0, 0}, {0, 1, 0}, {0, 0, 1}}}
        : Matrix3{{{1.30, 0.15, 0.00},
                   {0.15, 0.75, 0.08},
                   {0.00, 0.08, 1.10}}};
    for (int i = 0; i < 2000; ++i) {
        const double z = hemisphere ? uniform(random) : 2.0 * uniform(random) - 1.0;
        const double angle = 6.283185307179586 * uniform(random);
        const double xy = std::sqrt(1.0 - z * z);
        const double unit[3] = {xy * std::cos(angle), xy * std::sin(angle), z};
        double raw[3] = {12.0, -8.0, 4.0};
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col)
                raw[row] += transform[row][col] * field * unit[col];
            raw[row] += noise(random);
        }
        if (bad_holdout && i % 5 == 0) raw[0] += 25.0;
        samples.push_back({std::chrono::steady_clock::time_point{} +
                               std::chrono::milliseconds(50 * i),
                           {raw[0], raw[1], raw[2]}});
    }
    return samples;
}

std::string read_text(const std::filesystem::path& file) {
    std::ifstream input(file);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

} // namespace

int main() {
    constexpr Matrix3 identity{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    constexpr Matrix3 reflected{{{1, 0, 0}, {0, -1, 0}, {0, 0, 1}}};
    const auto samples = generate();
    Calibration fit;
    std::string error;
    require(fit_magnetometer_calibration(samples, "none", 44.9, identity, fit, error),
            error.c_str());
    require(fit.samples >= kCalibrationMinIndependent && fit.rms_uT < 1.0, "Synthetic fit quality");
    // Exactly the new minimum, balanced across all eight octants, must both
    // fit and pass save validation (a separate gate from the fitter).
    std::array<int, 8> balanced_counts{};
    std::vector<CalibrationSample> balanced;
    for (const auto& sample : samples) {
        const auto raw = sample.raw_magnetic_uT;
        const int octant = (raw.x >= 12.0 ? 1 : 0) |
                           (raw.y >= -8.0 ? 2 : 0) |
                           (raw.z >= 4.0 ? 4 : 0);
        if (balanced_counts[octant] < 25) {
            balanced.push_back(sample);
            ++balanced_counts[octant];
        }
    }
    require(balanced.size() == kCalibrationMinIndependent, "Could not balance minimum capture");
    Calibration minimum_fit;
    require(fit_magnetometer_calibration(balanced, "none", 44.9, identity,
                                         minimum_fit, error), error.c_str());
    const auto minimum_file = std::filesystem::temp_directory_path() /
                              ("zerosky_minimum_calibration_" + std::to_string(::getpid()) + ".json");
    require(save_calibration(minimum_file, minimum_fit,
                             {"2026-10-20T12:00:00Z", 36.05, 14.24}, error), error.c_str());
    std::filesystem::remove(minimum_file);
    std::filesystem::remove(minimum_file.string() + ".bak");
    require(std::abs(fit.offset_uT.x - 12.0) < 1.0 &&
            std::abs(fit.offset_uT.y + 8.0) < 1.0 &&
            std::abs(fit.offset_uT.z - 4.0) < 1.0, "Hard-iron offset recovery");
    double squared_error = 0;
    for (const auto& sample : samples) {
        const auto corrected = calibrate_magnetometer(fit, sample.raw_magnetic_uT);
        const double norm = std::sqrt(corrected.x * corrected.x +
                                      corrected.y * corrected.y +
                                      corrected.z * corrected.z);
        squared_error += (norm - 44.9) * (norm - 44.9);
    }
    require(std::sqrt(squared_error / samples.size()) < 1.0,
            "Soft-iron correction recovery");

    // A stationary start must not dominate the preview centre used by the
    // capture dialog. The first pose is deliberately disturbed; once the
    // device moves, the full sphere should determine the preview.
    auto stationary_start = std::vector<CalibrationSample>(2000, samples.front());
    for (auto& sample : stationary_start) sample.raw_magnetic_uT.x += 18.0;
    auto incomplete = stationary_start;
    incomplete.insert(incomplete.end(), samples.begin(), samples.begin() + 98);
    stationary_start.insert(stationary_start.end(), samples.begin(), samples.end());
    zerosky::sensors::Vec3 preview_centre{};
    require(!estimate_magnetometer_centre(incomplete, 44.9, preview_centre),
            "Repeated stationary reads counted as independent preview poses");
    require(estimate_magnetometer_centre(stationary_start, 44.9, preview_centre),
            "Stationary start prevented centre preview");
    require(std::abs(preview_centre.x - 12.0) < 2.0 &&
            std::abs(preview_centre.y + 8.0) < 2.0 &&
            std::abs(preview_centre.z - 4.0) < 2.0,
            "Stationary start biased centre preview");

    Calibration reflected_fit;
    require(fit_magnetometer_calibration(samples, "none", 44.9, reflected,
                                         reflected_fit, error), error.c_str());
    const auto raw = samples.front().raw_magnetic_uT;
    const auto corrected_identity = calibrate_magnetometer(fit, raw);
    const auto corrected_reflected = calibrate_magnetometer(reflected_fit, raw);
    require(std::abs(corrected_reflected.x - corrected_identity.x) < 1e-9 &&
            std::abs(corrected_reflected.y + corrected_identity.y) < 1e-9 &&
            std::abs(corrected_reflected.z - corrected_identity.z) < 1e-9,
            "Reflected magnetometer frame not applied");

    auto too_few = samples;
    too_few.resize(kCalibrationMinIndependent - 1);
    require(!fit_magnetometer_calibration(too_few, "none", 44.9, identity, fit, error),
            "Accepted fewer than 200 samples");
    require(!fit_magnetometer_calibration(generate(true), "none", 44.9,
                                         identity, fit, error),
            "Accepted incomplete octants");
    require(!fit_magnetometer_calibration(generate(false, true), "none", 44.9,
                                         identity, fit, error),
            "Accepted axis ratio above four");
    require(!fit_magnetometer_calibration(generate(false, false, true), "none", 44.9,
                                         identity, fit, error),
            "Accepted large held-out residual");

    require(fit_magnetometer_calibration(samples, "none", 44.9, identity, fit, error),
            error.c_str());
    const auto directory = std::filesystem::temp_directory_path() /
                           ("zerosky_calibration_test_" + std::to_string(::getpid()));
    std::filesystem::create_directories(directory);
    const auto file = directory / "magnetometer-calibration.json";
    const CalibrationMetadata metadata{"2026-10-20T12:00:00Z", 36.05, 14.24};
    const auto reflected_file = directory / "reflected-calibration.json";
    require(save_calibration(reflected_file, reflected_fit, metadata, error), error.c_str());
    Calibration reflected_loaded;
    require(load_calibration(reflected_file, "none", reflected_loaded, error), error.c_str());
    require(reflected_loaded.sensor_to_body == reflected,
            "Reflected magnetometer frame did not round-trip");
    require(save_calibration(file, fit, metadata, error), error.c_str());
    Calibration loaded;
    require(load_calibration(file, "none", loaded, error), error.c_str());
    require(std::abs(loaded.offset_uT.x - fit.offset_uT.x) < 1e-9,
            "Saved profile did not round-trip");
    Calibration cap = fit;
    cap.configuration = "cap";
    cap.offset_uT.x += 3.0;
    require(save_calibration(file, cap, metadata, error), error.c_str());
    require(std::filesystem::exists(file.string() + ".bak"), "Previous file not backed up");
    require(load_calibration(file, "none", loaded, error), "First profile was discarded");
    require(load_calibration(file, "cap", loaded, error), "Second profile was not saved");
    require(std::abs(loaded.offset_uT.x - cap.offset_uT.x) < 1e-9,
            "Wrong profile selected");
    const std::string before_failure = read_text(file);
    cap.sensor_to_body[0][0] = 0.5;
    require(!save_calibration(file, cap, metadata, error), "Accepted invalid frame");
    require(read_text(file) == before_failure, "Failed save changed the profile file");
    std::filesystem::remove_all(directory);
    return 0;
}
