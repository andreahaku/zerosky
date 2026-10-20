/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "sensor_source.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace zerosky::orientation {

struct GeomagneticField {
    // WMM components converted from nT in north/east/down to uT in east/north/up.
    sensors::Vec3 enu_uT;
    double total_uT = 0.0;
    double declination_deg = 0.0; // East-positive, relative to true north.
    double inclination_deg = 0.0; // Down-positive.
};

class GeomagneticModel {
public:
    static std::unique_ptr<GeomagneticModel> load(const std::filesystem::path& cof,
                                                  std::string& error);
    ~GeomagneticModel();
    GeomagneticModel(const GeomagneticModel&) = delete;
    GeomagneticModel& operator=(const GeomagneticModel&) = delete;

    // Height is metres above the WGS84 ellipsoid. Date is UTC decimal year.
    // WMM2025 is accepted from 2025.0 inclusive to 2030.0 exclusive.
    std::optional<GeomagneticField> field(double latitude_deg, double longitude_deg,
                                           double height_wgs84_m, double decimal_year) const;

private:
    struct Impl;
    explicit GeomagneticModel(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

// Convert a UTC calendar day to WMM's decimal-year input at 00:00 UTC.
std::optional<double> decimal_year_utc(int year, int month, int day);

} // namespace zerosky::orientation
