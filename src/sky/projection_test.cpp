/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "projection.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace zerosky::sky;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(double a, double b, double tol = 1e-6) { return std::abs(a - b) < tol; }

constexpr int kW = 320;
constexpr int kH = 170;

void centre_and_edges() {
    const Camera cam{90.0, 20.0, 0.0, 60.0};
    const Projection p(cam, kW, kH);
    const auto c = p.project(90.0, 20.0);
    require(c.visible && near(c.x, 160.0) && near(c.y, 85.0), "centre not at screen centre");
    // Half the field of view to the right, on the horizon of a level camera.
    const Projection level({0.0, 0.0, 0.0, 60.0}, kW, kH);
    const auto right = level.project(30.0, 0.0);
    require(near(right.x, 320.0) && near(right.y, 85.0), "fov edge not at the screen edge");
    // Higher altitude is up on the screen, east of north is right.
    require(level.project(0.0, 10.0).y < 85.0, "up is not up");
    require(level.project(10.0, 0.0).x > 160.0, "east is not right");
    // Behind the camera is not visible.
    require(!level.project(180.0, 0.0).visible, "point behind is visible");
    // A level camera has the horizon across the middle row.
    require(level.below_horizon(10.0, 100.0) && !level.below_horizon(10.0, 70.0),
            "horizon not across the middle");
}

void roll_turns_clockwise() {
    // Rolled 90 degrees, a point above the centre appears to the right.
    const Projection p({0.0, 0.0, 90.0, 60.0}, kW, kH);
    const auto up = p.project(0.0, 10.0);
    require(up.x > 160.0 && near(up.y, 85.0, 1e-6), "roll does not turn clockwise");
}

void guidance_points_the_way() {
    const Camera cam{0.0, 30.0, 0.0, 60.0};
    const auto g_right = guide(cam, 20.0, 30.0);
    require(g_right.screen_bearing_deg > 80.0 && g_right.screen_bearing_deg < 100.0,
            "target to the east is not to the right");
    const auto g_up = guide(cam, 0.0, 50.0);
    require(near(g_up.separation_deg, 20.0, 1e-9) && near(g_up.screen_bearing_deg, 0.0, 1e-9),
            "target above is not up");
    const auto g_here = guide(cam, 0.0, 30.0);
    require(g_here.separation_deg < 1e-6, "target on the centre is not centred");
}

void offset_turns_with_roll() {
    const Camera level{90.0, 20.0, 0.0, 60.0};
    const Camera same = offset_camera(level, 0.0, 0.0);
    require(near(same.azimuth_deg, 90.0, 1e-9) && near(same.altitude_deg, 20.0, 1e-9) &&
                near(same.roll_deg, 0.0, 1e-9),
            "zero offset changes the camera");
    // Level: an offset to the right turns the azimuth, not the altitude.
    const Camera right = offset_camera(level, 2.0, 0.0);
    require(right.azimuth_deg > 91.5 && near(right.altitude_deg, 20.0, 0.1), "right offset wrong");
    // Rolled 90 degrees clockwise, screen right looks up the sky (see
    // roll_turns_clockwise), so the same offset raises the altitude instead.
    const Camera rolled{90.0, 20.0, 90.0, 60.0};
    const Camera rolled_right = offset_camera(rolled, 2.0, 0.0);
    require(rolled_right.altitude_deg > 21.5 && near(rolled_right.azimuth_deg, 90.0, 0.2),
            "offset does not turn with the roll");
    // The new camera centre is where the old camera saw the offset point.
    const Projection old_view(rolled, kW, kH);
    const auto p = old_view.project(rolled_right.azimuth_deg, rolled_right.altitude_deg);
    const double expected_dx = (kW / 2.0) / std::tan(30.0 * M_PI / 180.0) * std::tan(2.0 * M_PI / 180.0);
    require(near(p.x - kW / 2.0, expected_dx, 0.05) && near(p.y, kH / 2.0, 0.05),
            "offset point not on screen right");
}

} // namespace

int main() {
    try {
        centre_and_edges();
        roll_turns_clockwise();
        guidance_points_the_way();
        offset_turns_with_roll();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("projection_test: ok");
    return 0;
}
