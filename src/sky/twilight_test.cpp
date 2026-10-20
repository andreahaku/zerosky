/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "twilight.h"

#include "astronomy.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>

using namespace zerosky::sky;

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

// The engine gives apparent altitude; construct it from geometric altitude so
// that the test checks the public input convention as well as phase boundaries.
Twilight at_geometric(double altitude_deg) {
    return twilight_from_sun_altitude(
        altitude_deg + Astronomy_Refraction(REFRACTION_NORMAL, altitude_deg));
}

void boundaries() {
    require(at_geometric(20.0).level == TwilightLevel::Day, "day at noon");
    require(at_geometric(-0.5).level == TwilightLevel::Day, "solar disc at horizon");
    require(at_geometric(-1.0).level == TwilightLevel::Civil, "civil after sunset");
    require(at_geometric(-5.9).level == TwilightLevel::Civil, "civil dark edge");
    require(at_geometric(-6.1).level == TwilightLevel::Nautical, "nautical after -6");
    require(at_geometric(-12.1).level == TwilightLevel::Astronomical,
            "astronomical after -12");
    require(at_geometric(-18.1).level == TwilightLevel::Night, "night after -18");
    require(std::fabs(at_geometric(-12.0).geometric_sun_altitude_deg + 12.0) < 1e-8,
            "inverse refraction did not recover geometric altitude");
}

void visibility_curve() {
    require(at_geometric(30.0).limiting_star_magnitude < -1.5,
            "daylight should hide every catalog star");
    require(at_geometric(-20.0).limiting_star_magnitude == 6.0,
            "full catalog should be available at night");
    double previous = at_geometric(0.0).limiting_star_magnitude;
    for (double altitude = -0.25; altitude >= -20.0; altitude -= 0.25) {
        const double current = at_geometric(altitude).limiting_star_magnitude;
        require(current >= previous - 1e-10, "visibility must grow as the Sun descends");
        previous = current;
    }
}

void invalid_input() {
    for (double altitude : {std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::infinity(), -91.0, 91.0}) {
        const auto twilight = twilight_from_sun_altitude(altitude);
        require(twilight.level == TwilightLevel::Unknown, "invalid solar altitude accepted");
        require(twilight.limiting_star_magnitude < -1.5,
                "invalid solar altitude exposed stars");
    }
}

} // namespace

int main() {
    try {
        boundaries();
        visibility_curve();
        invalid_input();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("twilight_test: ok");
    return 0;
}
