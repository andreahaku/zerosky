/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace zerosky::sky {

enum class TwilightLevel { Day, Civil, Nautical, Astronomical, Night, Unknown };

struct Twilight {
    TwilightLevel level{TwilightLevel::Unknown};
    // Faintest catalog star to draw. Smaller magnitudes are brighter.
    // This is a display heuristic, not a prediction of naked-eye visibility.
    // The curve uses -4 at sunset, +1 at -6 degrees, +4 at -12 degrees,
    // and +6 at -18 degrees of geometric solar altitude.
    double limiting_star_magnitude{-4.0};
    double geometric_sun_altitude_deg{0.0};
};

// Input is the Sun's apparent centre altitude in degrees, as returned by
// solar_system(). The standard refraction applied there is inverted before
// evaluating the USNO geometric twilight thresholds:
// https://aa.usno.navy.mil/faq/RST_defs
// Invalid input returns Unknown with a conservative star limit.
Twilight twilight_from_sun_altitude(double apparent_sun_altitude_deg);

} // namespace zerosky::sky
