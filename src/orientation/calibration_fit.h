/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "attitude_service.h"

#include <string>
#include <vector>

namespace zerosky::orientation {

// Fits hard/soft iron from raw BMM150 samples captured in varied 3D poses.
// The expected field is the local WMM total intensity in microtesla.
bool fit_magnetometer_calibration(const std::vector<CalibrationSample>& samples,
                                  const std::string& configuration,
                                  double expected_field_uT,
                                  const Matrix3& sensor_to_body,
                                  Calibration& result,
                                  std::string& error);

} // namespace zerosky::orientation
