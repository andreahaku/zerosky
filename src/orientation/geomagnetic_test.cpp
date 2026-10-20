/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "geomagnetic.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

using zerosky::orientation::GeomagneticModel;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void near(double actual, double expected, double tolerance, const char* message) {
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << message << ": got " << actual << ", expected " << expected << '\n';
        std::exit(1);
    }
}

} // namespace

int main(int argc, char** argv) {
    require(argc == 2, "Expected WMM.COF path");
    std::string error;
    auto model = GeomagneticModel::load(argv[1], error);
    require(model != nullptr, error.c_str());

    // NOAA WMM2025 test values, height above WGS84 ellipsoid, X/Y/Z in nT.
    // https://www.ncei.noaa.gov/sites/default/files/2025-02/WMM2025testvalues.pdf
    const auto polar = model->field(80.0, 0.0, 0.0, 2025.0);
    require(polar.has_value(), "No polar result");
    near(polar->enu_uT.x, 0.1459, 0.0002, "Polar east");
    near(polar->enu_uT.y, 6.5216, 0.0002, "Polar north");
    near(polar->enu_uT.z, -54.7915, 0.0002, "Polar up");
    near(polar->total_uT, 55.1785, 0.0002, "Polar total");
    near(polar->declination_deg, 1.28, 0.02, "Polar declination");

    const auto equator = model->field(0.0, 120.0, 0.0, 2027.5);
    require(equator.has_value(), "No equator result");
    near(equator->enu_uT.x, -0.1674, 0.0002, "Equator east");
    near(equator->enu_uT.y, 39.7016, 0.0002, "Equator north");
    near(equator->enu_uT.z, 10.3818, 0.0002, "Equator up");
    near(equator->total_uT, 41.0369, 0.0002, "Equator total");
    near(equator->declination_deg, -0.24, 0.02, "Equator declination");

    const auto elevated = model->field(-80.0, -120.0, 100000.0, 2027.5);
    require(elevated.has_value(), "No elevated result");
    near(elevated->enu_uT.x, 14.7601, 0.0002, "Elevated east");
    near(elevated->enu_uT.y, 5.9840, 0.0002, "Elevated north");
    near(elevated->enu_uT.z, 49.3177, 0.0002, "Elevated up");
    near(elevated->declination_deg, 67.93, 0.02, "Elevated declination");

    require(!model->field(91.0, 0.0, 0.0, 2025.0), "Accepted invalid latitude");
    require(!model->field(0.0, 0.0, 0.0, 2030.0), "Accepted expired model date");
    require(!model->field(0.0, 0.0, 0.0, std::nan("")), "Accepted NaN date");
    const auto day = zerosky::orientation::decimal_year_utc(2028, 3, 1);
    require(day && std::abs(*day - (2028.0 + 60.0 / 366.0)) < 1e-12,
            "Leap-year decimal date");
    require(!zerosky::orientation::decimal_year_utc(2027, 2, 29), "Accepted invalid day");
    return 0;
}
