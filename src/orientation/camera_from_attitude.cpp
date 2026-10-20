/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "camera_from_attitude.h"

#include <cmath>

namespace zerosky::orientation {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg = 180.0 / kPi;

bool finite(sensors::Vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

double dot(sensors::Vec3 a, sensors::Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double norm(sensors::Vec3 v) { return std::sqrt(dot(v, v)); }

sensors::Vec3 scaled(sensors::Vec3 v, double factor) {
    return {v.x * factor, v.y * factor, v.z * factor};
}

double wrapped_azimuth(double degrees) {
    double result = std::fmod(degrees, 360.0);
    if (result < 0.0) result += 360.0;
    return result;
}

} // namespace

std::optional<sky::Camera> camera_from_attitude(
    const AttitudeSnapshot& snapshot, const FrameMapping& mapping,
    const sky::Camera& previous_camera) {
    const Quaternion q = snapshot.body_to_enu;
    const double q_norm_squared = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    if (!mapping.verified || !std::isfinite(q_norm_squared) || q_norm_squared < 1e-12 ||
        !finite(mapping.pointing_body) || !finite(mapping.screen_up_body)) return std::nullopt;

    const double pointing_norm = norm(mapping.pointing_body);
    if (!std::isfinite(pointing_norm) || pointing_norm < 1e-9) return std::nullopt;
    const auto pointing = scaled(mapping.pointing_body, 1.0 / pointing_norm);
    const double along = dot(mapping.screen_up_body, pointing);
    const sensors::Vec3 up_tangent{mapping.screen_up_body.x - along * pointing.x,
                                   mapping.screen_up_body.y - along * pointing.y,
                                   mapping.screen_up_body.z - along * pointing.z};
    const double up_norm = norm(up_tangent);
    if (!std::isfinite(up_norm) || up_norm < 1e-3) return std::nullopt;

    const auto forward = Mahony::rotate(q, pointing);
    const auto up = Mahony::rotate(q, scaled(up_tangent, 1.0 / up_norm));
    if (!finite(forward) || !finite(up)) return std::nullopt;

    const double horizontal = std::hypot(forward.x, forward.y);
    const double altitude = std::atan2(forward.z, horizontal);
    double azimuth = 0.0;
    if (horizontal < 1e-4) {
        // The physical screen basis remains defined, even when yaw is not.
        azimuth = std::isfinite(previous_camera.azimuth_deg)
                      ? wrapped_azimuth(previous_camera.azimuth_deg) : 0.0;
    } else {
        azimuth = wrapped_azimuth(std::atan2(forward.x, forward.y) * kDeg);
    }

    // Match Projection::camera_basis exactly: u = u0*cos(roll) - r0*sin(roll).
    const double az = azimuth / kDeg;
    const sensors::Vec3 r0{std::cos(az), -std::sin(az), 0.0};
    const sensors::Vec3 u0{-std::sin(altitude) * std::sin(az),
                            -std::sin(altitude) * std::cos(az), std::cos(altitude)};
    const double roll = std::atan2(-dot(up, r0), dot(up, u0)) * kDeg;

    sky::Camera camera;
    camera.azimuth_deg = azimuth;
    camera.altitude_deg = altitude * kDeg;
    camera.roll_deg = roll;
    camera.fov_deg = std::isfinite(previous_camera.fov_deg) &&
                             previous_camera.fov_deg > 0.0 && previous_camera.fov_deg < 180.0
                         ? previous_camera.fov_deg : 60.0;
    return camera;
}

} // namespace zerosky::orientation
