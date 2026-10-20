/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "camera_from_attitude.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace zerosky::orientation;
using zerosky::sky::Projection;

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr int kWidth = 320;
constexpr int kHeight = 170;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(double a, double b, double tolerance = 1e-6) {
    return std::abs(a - b) < tolerance;
}

Quaternion around(zerosky::sensors::Vec3 axis, double degrees) {
    const double half = degrees * kPi / 360.0;
    return {std::cos(half), axis.x * std::sin(half), axis.y * std::sin(half),
            axis.z * std::sin(half)};
}

Quaternion multiply(Quaternion a, Quaternion b) {
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

FrameMapping synthetic_mapping() {
    FrameMapping mapping;
    mapping.pointing_body = {0, 1, 0};
    mapping.screen_up_body = {0, 0, 1};
    mapping.verified = true;
    return mapping;
}

void check_projection(const zerosky::sky::Camera& camera,
                      zerosky::sensors::Vec3 forward, zerosky::sensors::Vec3 screen_up) {
    const Projection projection(camera, kWidth, kHeight);
    auto project_vec = [&projection](zerosky::sensors::Vec3 v) {
        const double az = std::atan2(v.x, v.y) * 180.0 / kPi;
        const double alt = std::atan2(v.z, std::hypot(v.x, v.y)) * 180.0 / kPi;
        return projection.project(az, alt);
    };
    const auto centre = project_vec(forward);
    require(centre.visible && near(centre.x, 160.0, 0.05) &&
                near(centre.y, 85.0, 0.05),
            "pointing axis not at screen centre: " + std::to_string(centre.x) + "," +
                std::to_string(centre.y));
    const zerosky::sensors::Vec3 above{forward.x + 0.1 * screen_up.x,
                                       forward.y + 0.1 * screen_up.y,
                                       forward.z + 0.1 * screen_up.z};
    const auto upper = project_vec(above);
    require(upper.visible && upper.y < 85.0, "screen-up axis does not project upward");
}

void cardinal_and_roll_cases() {
    const FrameMapping mapping = synthetic_mapping();
    AttitudeSnapshot snapshot;
    zerosky::sky::Camera previous;
    previous.fov_deg = 42.0;

    const auto north = camera_from_attitude(snapshot, mapping, previous);
    require(north && near(north->azimuth_deg, 0.0) && near(north->altitude_deg, 0.0) &&
                near(north->roll_deg, 0.0) && near(north->fov_deg, 42.0),
            "level north camera wrong");
    check_projection(*north, {0, 1, 0}, {0, 0, 1});
    require(Projection(*north, kWidth, kHeight).project(0, 10).y < 85,
            "higher altitude not above on level screen");

    snapshot.body_to_enu = multiply(around({0, 0, 1}, -90), around({1, 0, 0}, 30));
    const auto east = camera_from_attitude(snapshot, mapping);
    require(east && near(east->azimuth_deg, 90.0) &&
                near(east->altitude_deg, 30.0) && near(east->roll_deg, 0.0),
            "east at 30 degrees camera wrong");
    check_projection(*east, Mahony::rotate(snapshot.body_to_enu, mapping.pointing_body),
                     Mahony::rotate(snapshot.body_to_enu, mapping.screen_up_body));
    require(Projection(*east, kWidth, kHeight).project(90, 35).y < 85,
            "higher altitude not above when facing east");

    snapshot.body_to_enu = around({0, 1, 0}, -90);
    const auto rolled = camera_from_attitude(snapshot, mapping);
    require(rolled && near(rolled->azimuth_deg, 0.0) &&
                near(rolled->altitude_deg, 0.0) && near(rolled->roll_deg, 90.0),
            "clockwise roll sign wrong");
    check_projection(*rolled, {0, 1, 0}, {-1, 0, 0});
    const auto zenith = Projection(*rolled, kWidth, kHeight).project(0, 10);
    require(zenith.x > 160.0 && near(zenith.y, 85.0),
            "clockwise roll does not move a high target right");
}

void poles_and_invalid_mapping() {
    FrameMapping mapping = synthetic_mapping();
    AttitudeSnapshot snapshot;
    zerosky::sky::Camera previous{123.4, 0.0, 0.0, 37.0};

    snapshot.body_to_enu = around({1, 0, 0}, 89.999);
    const auto zenith = camera_from_attitude(snapshot, mapping, previous);
    require(zenith && near(zenith->azimuth_deg, 123.4) &&
                zenith->altitude_deg > 89.99 && near(zenith->fov_deg, 37.0),
            "zenith fallback lost previous azimuth");
    check_projection(*zenith, Mahony::rotate(snapshot.body_to_enu, mapping.pointing_body),
                     Mahony::rotate(snapshot.body_to_enu, mapping.screen_up_body));

    snapshot.body_to_enu = around({1, 0, 0}, -89.999);
    const auto nadir = camera_from_attitude(snapshot, mapping, previous);
    require(nadir && near(nadir->azimuth_deg, 123.4) && nadir->altitude_deg < -89.99,
            "nadir fallback lost previous azimuth");

    mapping.verified = false;
    require(!camera_from_attitude(snapshot, mapping), "unverified mapping accepted");
    mapping.verified = true;
    mapping.screen_up_body = mapping.pointing_body;
    require(!camera_from_attitude(snapshot, mapping), "parallel screen-up accepted");
    mapping = synthetic_mapping();
    snapshot.body_to_enu = {0, 0, 0, 0};
    require(!camera_from_attitude(snapshot, mapping), "zero quaternion accepted");
}

} // namespace

int main() {
    try {
        cardinal_and_roll_cases();
        poles_and_invalid_mapping();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("camera_from_attitude_test: ok");
    return 0;
}
