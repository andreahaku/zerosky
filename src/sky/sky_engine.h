/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <ctime>
#include <vector>

namespace zerosky::sky {

// Where the observer stands. Latitude and longitude in degrees (east and north
// positive), height above sea level in metres.
struct Observer {
    double latitude_deg{0.0};
    double longitude_deg{0.0};
    double height_m{0.0};
};

enum class Body { Sun, Moon, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune };

// Horizontal coordinates: azimuth 0 = north, 90 = east; altitude above the
// horizon, with standard atmospheric refraction.
struct Horizontal {
    double azimuth_deg{0.0};
    double altitude_deg{0.0};
};

struct BodyPosition {
    Body body{Body::Sun};
    const char* name{""};
    Horizontal horizontal;
    double ra_hours{0.0};      // right ascension, equator of date
    double dec_deg{0.0};       // declination, equator of date
    double distance_au{0.0};
    double magnitude{0.0};     // visual magnitude, smaller is brighter
    double phase_fraction{1.0}; // illuminated fraction of the disc
};

// Sun, Moon and the planets seen from `observer` at `utc` (Unix time, may carry
// a fraction of a second). A body whose computation fails is left out.
std::vector<BodyPosition> solar_system(double utc, const Observer& observer);

// A star from a J2000 catalog (right ascension in hours, declination in
// degrees) seen from `observer` at `utc`. Precession and nutation to the
// equator of date are applied. Proper motion is not.
Horizontal star_horizontal(double ra_j2000_hours, double dec_j2000_deg, double utc,
                           const Observer& observer);

} // namespace zerosky::sky
