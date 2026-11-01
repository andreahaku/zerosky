/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "calibration_flow.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <unistd.h>

using namespace zerosky::orientation;

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<CalibrationSample> sphere(bool top_only = false) {
    std::mt19937 random(117);
    std::uniform_real_distribution<double> uniform(0, 1);
    std::vector<CalibrationSample> samples;
    for (int i = 0; i < 1400; ++i) {
        const double z = top_only ? uniform(random) : 2 * uniform(random) - 1;
        const double angle = 6.283185307179586 * uniform(random);
        const double xy = std::sqrt(1 - z * z);
        samples.push_back({std::chrono::steady_clock::time_point{} +
                               std::chrono::milliseconds(i * 50),
                           {18 + 48 * xy * std::cos(angle),
                            -7 + 37 * xy * std::sin(angle), 22 + 45 * z}});
    }
    return samples;
}

} // namespace

int main() {
    try {
        constexpr Matrix3 measured{{{-1, 0, 0}, {0, 1, 0}, {0, 0, -1}}};
        const auto samples = sphere();
        auto duplicates = samples;
        duplicates.clear();
        for (size_t i = 0; i < 400; ++i) {
            duplicates.push_back(samples[i]);
            duplicates.push_back(samples[i]);
        }
        require(capture_coverage(duplicates, 44.9).independent == 400,
                "Repeated static samples counted as independent");
        CaptureCoverage almost;
        almost.independent = kCalibrationMinIndependent;
        almost.octant_counts.fill(kCalibrationMinPerOctant);
        almost.octant_counts[7] = kCalibrationMinPerOctant - 1;
        almost.octants = 7;
        require(!almost.complete(), "Fourteen samples in one octant passed");
        ++almost.octant_counts[7];
        almost.octants = 8;
        require(almost.complete(), "Fifteen samples per octant did not pass");
        almost.octants = 7;
        almost.octant_counts.fill(kCalibrationMinPerOctant);
        almost.octant_counts[0] = 0;
        require(missing_pose_hint(almost, measured, 60.0) == MissingPoseHint::ScreenDown,
                "Measured BMM150 frame did not suggest screen down");
        almost.octant_counts[0] = kCalibrationMinPerOctant;
        almost.octant_counts[4] = 0;
        require(missing_pose_hint(almost, measured, 60.0) == MissingPoseHint::ScreenUp,
                "Opposite octant did not suggest screen up");
        require(missing_pose_hint(almost, measured, -60.0) == MissingPoseHint::ScreenDown,
                "Southern WMM inclination did not reverse the hint");
        require(missing_pose_hint(almost, measured, 0.0) == MissingPoseHint::TurnAllSides,
                "Horizontal magnetic field gave a misleading vertical hint");
        CaptureCoverage unclassified;
        unclassified.independent = 72;
        require(missing_pose_hint(unclassified, measured, 60.0) == MissingPoseHint::TurnAllSides,
                "Hint appeared before the hard-iron centre was estimated");
        require(capture_coverage(sphere(true), 44.9).octants < 8,
                "A hemisphere passed the eight-octant gate");
        require(capture_coverage(samples, 44.9).complete(),
                "Full 3D rotation did not complete coverage");

        const auto directory = std::filesystem::temp_directory_path() /
                               ("zerosky_calibration_flow_" + std::to_string(::getpid()));
        std::filesystem::remove_all(directory);
        std::filesystem::create_directories(directory);
        const auto file = directory / "magnetometer-calibration.json";
        FrameMapping frame;
        Matrix3 resolved{};
        std::string error;
        require(!resolve_magnetic_frame(frame, file, resolved, error),
                "Unverified frame was accepted");
        frame.verified = true;
        require(!resolve_magnetic_frame(frame, file, resolved, error),
                "Schema-1 frame with no earlier profile was accepted");
        frame.mag_verified = true;
        frame.mag_to_body = measured;
        require(resolve_magnetic_frame(frame, file, resolved, error) && resolved == measured,
                "Schema-2 magnetic frame was not preferred");
        frame.mag_verified = false;
        const auto now = std::chrono::steady_clock::now();
        const CalibrationMetadata metadata{"2026-10-31T09:00:00Z", 36.05, 14.24};
        CalibrationFlow flow("cap", 44.9, measured, metadata, file, now);
        require(!flow.should_finish(now), "Empty capture stopped early");
        require(!flow.needs_preview(9) && flow.needs_preview(10),
                "Preview should wait for ten captured samples");
        const auto short_capture = std::vector<CalibrationSample>(
            duplicates.begin(), duplicates.begin() + 300);
        flow.update(short_capture);
        require(!flow.needs_preview(short_capture.size() + 9) &&
                    flow.needs_preview(short_capture.size() + 10),
                "Preview should wait for ten new samples");
        require(!flow.should_finish(now + std::chrono::seconds(5)),
                "Too few poses stopped capture");
        flow.update(samples);
        require(flow.should_finish(now + std::chrono::seconds(6)),
                "Complete coverage did not auto-stop");
        flow.finish(samples);
        require(flow.stage() == CalibrationStage::Ready && flow.result().rms_uT < 2.0,
                "Full capture did not reach review state");
        require(!std::filesystem::exists(file), "Fit wrote the profile before Enter");
        flow.cancel();
        require(flow.stage() == CalibrationStage::Cancelled && !std::filesystem::exists(file),
                "Esc/cancel wrote the profile");

        CalibrationFlow saved("cap", 44.9, measured, metadata, file, now);
        saved.update(samples);
        saved.finish(samples);
        require(saved.save(error), error.c_str());
        Calibration loaded;
        require(load_calibration(file, "cap", loaded, error) && loaded.sensor_to_body == measured,
                "Enter/save did not persist the active profile and magnetic frame");
        require(resolve_magnetic_frame(frame, file, resolved, error) && resolved == measured,
                "Schema-1 fallback did not load the earlier profile's magnetic frame");

        CalibrationFlow timeout("none", 44.9, measured, metadata, file, now);
        timeout.update(std::vector<CalibrationSample>(samples.begin(),
                           samples.begin() + static_cast<long>(kCalibrationMinIndependent - 1)));
        require(timeout.should_finish(now + std::chrono::seconds(120)),
                "Capture did not stop at 120 seconds");
        timeout.finish(std::vector<CalibrationSample>(samples.begin(),
                           samples.begin() + static_cast<long>(kCalibrationMinIndependent - 1)));
        require(timeout.stage() == CalibrationStage::Failed &&
                    timeout.error().find("200") != std::string::npos,
                "Failed fit did not expose a reason");
        require(load_calibration(file, "cap", loaded, error),
                "Failed fit replaced the earlier valid profile");
        std::filesystem::remove_all(directory);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("calibration_flow_test: ok");
    return 0;
}
