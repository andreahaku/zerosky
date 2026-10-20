/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "twilight.h"

#include "astronomy.h"

#include <cmath>

namespace zerosky::sky {
namespace {

// USNO sunrise/sunset convention: upper solar limb on the horizon, with
// standard refraction, puts the geometric centre 50 arcminutes below it.
constexpr double kSunriseAltitude = -50.0 / 60.0;
constexpr double kCivilEnd = -6.0;
constexpr double kNauticalEnd = -12.0;
constexpr double kAstronomicalEnd = -18.0;

double interpolate(double x, double x0, double y0, double x1, double y1) {
    return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

} // namespace

Twilight twilight_from_sun_altitude(double apparent_sun_altitude_deg) {
    if (!std::isfinite(apparent_sun_altitude_deg) ||
        apparent_sun_altitude_deg < -90.0 || apparent_sun_altitude_deg > 90.0) {
        return {};
    }

    const double geometric = apparent_sun_altitude_deg +
        Astronomy_InverseRefraction(REFRACTION_NORMAL, apparent_sun_altitude_deg);
    if (!std::isfinite(geometric)) return {};

    if (geometric >= kSunriseAltitude) {
        return {TwilightLevel::Day, -4.0, geometric};
    }
    if (geometric >= kCivilEnd) {
        return {TwilightLevel::Civil,
                interpolate(geometric, kSunriseAltitude, -4.0, kCivilEnd, 1.0), geometric};
    }
    if (geometric >= kNauticalEnd) {
        return {TwilightLevel::Nautical,
                interpolate(geometric, kCivilEnd, 1.0, kNauticalEnd, 4.0), geometric};
    }
    if (geometric >= kAstronomicalEnd) {
        return {TwilightLevel::Astronomical,
                interpolate(geometric, kNauticalEnd, 4.0, kAstronomicalEnd, 6.0), geometric};
    }
    return {TwilightLevel::Night, 6.0, geometric};
}

} // namespace zerosky::sky
