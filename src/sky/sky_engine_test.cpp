/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

// Checks the sky engine against values that follow from geometry alone, so the
// test does not depend on a second ephemeris.

#include "sky_engine.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace zerosky::sky;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr Observer kGozo{36.05, 14.24, 50.0};
// 2026-06-21 00:00 UTC, the day of the June solstice.
constexpr double kSolsticeDay = 1782000000.0;

const BodyPosition& find(const std::vector<BodyPosition>& bodies, Body body) {
    for (const auto& pos : bodies) {
        if (pos.body == body) return pos;
    }
    throw std::runtime_error("body missing");
}

// At the June solstice the Sun culminates at 90 - latitude + 23.44 degrees.
void sun_culmination_at_solstice() {
    double best = -90.0;
    for (int minute = 0; minute < 24 * 60; ++minute) {
        const auto bodies = solar_system(kSolsticeDay + minute * 60.0, kGozo);
        best = std::max(best, find(bodies, Body::Sun).horizontal.altitude_deg);
    }
    const double expected = 90.0 - kGozo.latitude_deg + 23.44;
    std::printf("sun culmination %.3f, expected %.3f\n", best, expected);
    require(std::abs(best - expected) < 0.1, "Sun culmination off");
}

// Polaris sits within 0.75 degrees of the pole, so its altitude is close to the latitude.
void polaris_tracks_latitude() {
    const double ra = 2.0 + 31.0 / 60.0 + 49.09 / 3600.0;
    const double dec = 89.0 + 15.0 / 60.0 + 50.8 / 3600.0;
    for (int hour = 0; hour < 24; hour += 3) {
        const Horizontal h = star_horizontal(ra, dec, kSolsticeDay + hour * 3600.0, kGozo);
        require(std::abs(h.altitude_deg - kGozo.latitude_deg) < 0.8, "Polaris altitude off");
        const double az = h.azimuth_deg > 180.0 ? h.azimuth_deg - 360.0 : h.azimuth_deg;
        require(std::abs(az) < 1.5, "Polaris azimuth off");
    }
}

void bodies_in_physical_ranges() {
    const auto bodies = solar_system(kSolsticeDay, kGozo);
    require(bodies.size() == 9, "expected 9 bodies");
    const double moon_au = find(bodies, Body::Moon).distance_au;
    require(moon_au > 0.00236 && moon_au < 0.00272, "Moon distance off");
    const double venus_mag = find(bodies, Body::Venus).magnitude;
    require(venus_mag > -4.9 && venus_mag < -3.7, "Venus magnitude off");
    const double sun_au = find(bodies, Body::Sun).distance_au;
    require(sun_au > 1.0 && sun_au < 1.02, "Sun distance at aphelion season off");
}

} // namespace

int main() {
    try {
        sun_culmination_at_solstice();
        polaris_tracks_latitude();
        bodies_in_physical_ranges();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("sky_engine_test: ok");
    return 0;
}
