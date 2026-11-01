/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "calibration_flow.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace zerosky::orientation {
namespace {

bool finite(sensors::Vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

double distance_squared(sensors::Vec3 a, sensors::Vec3 b) {
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) +
           (a.z - b.z) * (a.z - b.z);
}

} // namespace

MissingPoseHint missing_pose_hint(const CaptureCoverage& coverage,
                                  const Matrix3& mag_to_body, double inclination_deg) {
    unsigned classified = 0;
    for (const unsigned count : coverage.octant_counts) classified += count;
    if (coverage.independent == 0 || coverage.octants == 8 ||
        classified == 0 || !std::isfinite(inclination_deg) ||
        std::abs(inclination_deg) < 20.0)
        return MissingPoseHint::TurnAllSides;
    unsigned missing = 0;
    for (unsigned octant = 1; octant < 8; ++octant)
        if (coverage.octant_counts[octant] < coverage.octant_counts[missing]) missing = octant;
    const double sensor[3] = {missing & 1u ? 1.0 : -1.0,
                              missing & 2u ? 1.0 : -1.0,
                              missing & 4u ? 1.0 : -1.0};
    double body[3]{};
    for (int axis = 0; axis < 3; ++axis)
        for (int i = 0; i < 3; ++i) body[axis] += mag_to_body[axis][i] * sensor[i];
    // Prefer the screen normal on ties. Otherwise suggest the body edge whose
    // alignment with the mostly vertical field most helps the missing octant.
    int axis = 2;
    for (int i = 0; i < 2; ++i)
        if (std::abs(body[i]) > std::abs(body[axis]) + 0.01) axis = i;
    const bool edge_down = (body[axis] > 0.0) == (inclination_deg > 0.0);
    if (axis == 2) return edge_down ? MissingPoseHint::ScreenDown : MissingPoseHint::ScreenUp;
    if (axis == 0) return edge_down ? MissingPoseHint::KeyboardDown : MissingPoseHint::TopDown;
    return edge_down ? MissingPoseHint::RightDown : MissingPoseHint::LeftDown;
}

bool resolve_magnetic_frame(const FrameMapping& mapping,
                            const std::filesystem::path& calibration_file,
                            Matrix3& frame, std::string& error) {
    error.clear();
    if (!mapping.verified) {
        error = "Frame axes are not measured; use zerosky_probe";
        return false;
    }
    if (mapping.mag_verified) {
        frame = mapping.mag_to_body;
        return true;
    }
    Calibration previous;
    for (const char* profile : {"none", "cap", "usb_gps"}) {
        std::string ignored;
        if (load_calibration(calibration_file, profile, previous, ignored)) {
            frame = previous.sensor_to_body;
            return true;
        }
    }
    error = "Magnetometer axes are not measured; use zerosky_probe --save-frames";
    return false;
}

CaptureCoverage capture_coverage(const std::vector<CalibrationSample>& samples,
                                 double expected_field_uT) {
    CaptureCoverage coverage;
    std::vector<sensors::Vec3> independent;
    independent.reserve(samples.size());
    for (const auto& sample : samples) {
        const auto value = sample.raw_magnetic_uT;
        if (!finite(value) || (!independent.empty() &&
                               distance_squared(value, independent.back()) < 0.25)) continue;
        independent.push_back(value);
    }
    coverage.independent = independent.size();
    sensors::Vec3 centre{};
    if (!estimate_magnetometer_centre(samples, expected_field_uT, centre)) return coverage;
    for (const auto value : independent) {
        const unsigned octant = (value.x >= centre.x ? 1u : 0u) |
                                (value.y >= centre.y ? 2u : 0u) |
                                (value.z >= centre.z ? 4u : 0u);
        ++coverage.octant_counts[octant];
    }
    for (unsigned count : coverage.octant_counts)
        if (count >= kCalibrationMinPerOctant) ++coverage.octants;
    return coverage;
}

CalibrationFlow::CalibrationFlow(std::string hardware, double expected_field_uT,
                                 Matrix3 mag_to_body, CalibrationMetadata metadata,
                                 std::filesystem::path file,
                                 std::chrono::steady_clock::time_point started,
                                 double inclination_deg)
    : hardware_(std::move(hardware)), expected_field_uT_(expected_field_uT),
      mag_to_body_(mag_to_body), inclination_deg_(inclination_deg),
      metadata_(std::move(metadata)), file_(std::move(file)),
      started_(started) {}

void CalibrationFlow::update(const std::vector<CalibrationSample>& samples) {
    if (!needs_preview(samples.size())) return;
    // The ellipsoid-centre preview is moderately expensive on the A53.
    last_preview_count_ = samples.size();
    coverage_ = capture_coverage(samples, expected_field_uT_);
}

bool CalibrationFlow::needs_preview(size_t sample_count) const {
    return stage_ == CalibrationStage::Capturing &&
           sample_count >= last_preview_count_ + 10;
}

bool CalibrationFlow::should_finish(std::chrono::steady_clock::time_point now) const {
    return stage_ == CalibrationStage::Capturing &&
           (coverage_.complete() || now - started_ >= std::chrono::seconds(120));
}

int CalibrationFlow::seconds_left(std::chrono::steady_clock::time_point now) const {
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - started_).count();
    return static_cast<int>(std::clamp<std::int64_t>(120 - elapsed, 0, 120));
}

void CalibrationFlow::finish(const std::vector<CalibrationSample>& samples) {
    if (stage_ != CalibrationStage::Capturing) return;
    coverage_ = capture_coverage(samples, expected_field_uT_);
    if (!fit_magnetometer_calibration(samples, hardware_, expected_field_uT_,
                                      mag_to_body_, fitted_, error_)) {
        stage_ = CalibrationStage::Failed;
        return;
    }
    stage_ = CalibrationStage::Ready;
}

bool CalibrationFlow::save(std::string& error) {
    if (stage_ != CalibrationStage::Ready) {
        error = "Calibration is not ready to save";
        return false;
    }
    if (!save_calibration(file_, fitted_, metadata_, error)) {
        error_ = error;
        stage_ = CalibrationStage::Failed;
        return false;
    }
    stage_ = CalibrationStage::Saved;
    error_.clear();
    return true;
}

void CalibrationFlow::cancel() {
    if (stage_ != CalibrationStage::Saved) stage_ = CalibrationStage::Cancelled;
}

} // namespace zerosky::orientation
