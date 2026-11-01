/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "calibration_fit.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace zerosky::orientation {

struct CaptureCoverage {
    size_t independent = 0;
    std::array<unsigned, 8> octant_counts{};
    unsigned octants = 0; // An octant counts after kCalibrationMinPerOctant samples.
    bool complete() const { return independent >= kCalibrationMinIndependent && octants == 8; }
};

enum class MissingPoseHint {
    TurnAllSides, ScreenDown, ScreenUp, KeyboardDown, TopDown, RightDown, LeftDown
};

// Translate the least-covered magnetic octant through the measured sensor
// frame. A strong WMM inclination gives a useful gravity-relative pose cue.
MissingPoseHint missing_pose_hint(const CaptureCoverage& coverage,
                                  const Matrix3& mag_to_body, double inclination_deg);

// Preview the fit's 0.5 uT movement gate. Octants use the observed bounding-box
// centre until the ellipsoid fit can estimate hard iron; the fit makes the final check.
CaptureCoverage capture_coverage(const std::vector<CalibrationSample>& samples,
                                 double expected_field_uT);

// Prefer the per-device schema-2 mapping. A schema-1 mapping may use the
// validated global BMM150 frame from an older profile file, if one exists.
bool resolve_magnetic_frame(const FrameMapping& mapping,
                            const std::filesystem::path& calibration_file,
                            Matrix3& frame, std::string& error);

enum class CalibrationStage { Capturing, Ready, Failed, Saved, Cancelled };

// The UI owns this transaction. Only save() writes the profile; cancelling or
// failing a fit never replaces a previous valid calibration.
class CalibrationFlow {
public:
    CalibrationFlow(std::string hardware, double expected_field_uT, Matrix3 mag_to_body,
                    CalibrationMetadata metadata, std::filesystem::path file,
                    std::chrono::steady_clock::time_point started,
                    double inclination_deg = 0.0);

    void update(const std::vector<CalibrationSample>& samples);
    bool needs_preview(size_t sample_count) const;
    bool should_finish(std::chrono::steady_clock::time_point now) const;
    void finish(const std::vector<CalibrationSample>& samples);
    bool save(std::string& error);
    void cancel();

    CalibrationStage stage() const { return stage_; }
    const CaptureCoverage& coverage() const { return coverage_; }
    const Calibration& result() const { return fitted_; }
    const std::string& error() const { return error_; }
    double field_uT() const { return expected_field_uT_; }
    MissingPoseHint missing_pose() const {
        return missing_pose_hint(coverage_, mag_to_body_, inclination_deg_);
    }
    int seconds_left(std::chrono::steady_clock::time_point now) const;

private:
    std::string hardware_;
    double expected_field_uT_;
    Matrix3 mag_to_body_;
    double inclination_deg_;
    CalibrationMetadata metadata_;
    std::filesystem::path file_;
    std::chrono::steady_clock::time_point started_;
    CalibrationStage stage_ = CalibrationStage::Capturing;
    CaptureCoverage coverage_;
    size_t last_preview_count_ = 0;
    Calibration fitted_;
    std::string error_;
};

} // namespace zerosky::orientation
