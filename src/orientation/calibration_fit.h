/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "attitude_service.h"

#include <cstddef>
#include <string>
#include <vector>

namespace zerosky::orientation {

inline constexpr std::size_t kCalibrationMinIndependent = 200;
inline constexpr unsigned kCalibrationMinPerOctant = 15;

// Fits hard/soft iron from raw BMM150 samples captured in varied 3D poses.
// The expected field is the local WMM total intensity in microtesla.
bool fit_magnetometer_calibration(const std::vector<CalibrationSample>& samples,
                                  const std::string& configuration,
                                  double expected_field_uT,
                                  const Matrix3& sensor_to_body,
                                  Calibration& result,
                                  std::string& error);

// Preview the hard-iron centre for progress reporting. The final fitter still
// decides whether a capture is valid.
bool estimate_magnetometer_centre(const std::vector<CalibrationSample>& samples,
                                  double expected_field_uT, sensors::Vec3& centre_uT);

} // namespace zerosky::orientation
