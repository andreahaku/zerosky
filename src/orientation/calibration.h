/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "sensor_source.h"

#include <array>
#include <filesystem>
#include <string>

namespace zerosky::orientation {

using Matrix3 = std::array<std::array<double, 3>, 3>;

struct Calibration {
    std::string configuration;
    sensors::Vec3 offset_uT;
    Matrix3 soft_iron{};
    Matrix3 sensor_to_body{};
    double field_uT = 0.0;
    double rms_uT = 0.0;
    int samples = 0;
};

struct CalibrationMetadata {
    std::string measured_at_utc; // ISO 8601 UTC, supplied by the caller.
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
};

// Loads only the profile matching the active hardware configuration. A missing
// profile is an error, never an implicit fallback to another configuration.
bool load_calibration(const std::filesystem::path& file,
                      const std::string& configuration,
                      Calibration& result, std::string& error);

// Preserves other hardware profiles, writes a sibling temporary file, fsyncs
// it, keeps the previous complete file at .bak, then atomically renames.
bool save_calibration(const std::filesystem::path& file,
                      const Calibration& calibration,
                      const CalibrationMetadata& metadata,
                      std::string& error);

sensors::Vec3 calibrate_magnetometer(const Calibration& calibration,
                                    sensors::Vec3 raw_uT);

} // namespace zerosky::orientation
