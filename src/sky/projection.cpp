/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "projection.h"

#include <cmath>

namespace zerosky::sky {
namespace {

constexpr double kDeg = M_PI / 180.0;

double dot(const double a[3], const double b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Camera basis in ENU: forward to the screen centre, right and up on the screen.
void camera_basis(const Camera& c, double f[3], double r[3], double u[3]) {
    const double az = c.azimuth_deg * kDeg;
    const double alt = c.altitude_deg * kDeg;
    enu_from_horizontal(c.azimuth_deg, c.altitude_deg, f);
    // With no roll, right is horizontal and up is towards the zenith.
    const double r0[3] = {std::cos(az), -std::sin(az), 0.0};
    const double u0[3] = {-std::sin(alt) * std::sin(az), -std::sin(alt) * std::cos(az),
                          std::cos(alt)};
    const double roll = c.roll_deg * kDeg;
    const double cr = std::cos(roll);
    const double sr = std::sin(roll);
    for (int i = 0; i < 3; ++i) {
        r[i] = r0[i] * cr + u0[i] * sr;
        u[i] = u0[i] * cr - r0[i] * sr;
    }
}

} // namespace

void enu_from_horizontal(double azimuth_deg, double altitude_deg, double out[3]) {
    const double az = azimuth_deg * kDeg;
    const double alt = altitude_deg * kDeg;
    out[0] = std::cos(alt) * std::sin(az);
    out[1] = std::cos(alt) * std::cos(az);
    out[2] = std::sin(alt);
}

Projection::Projection(const Camera& camera, int width, int height)
    : focal_px_(width / 2.0 / std::tan(camera.fov_deg * kDeg / 2.0)),
      cx_(width / 2.0),
      cy_(height / 2.0),
      width_(width),
      height_(height) {
    camera_basis(camera, forward_, right_, up_);
}

ScreenPoint Projection::project(double azimuth_deg, double altitude_deg) const {
    double v[3];
    enu_from_horizontal(azimuth_deg, altitude_deg, v);
    const double depth = dot(v, forward_);
    if (depth <= 1e-6) return {};
    return {cx_ + focal_px_ * dot(v, right_) / depth, cy_ - focal_px_ * dot(v, up_) / depth, true};
}

bool Projection::on_screen(const ScreenPoint& p, double margin) const {
    return p.visible && p.x >= -margin && p.x < width_ + margin && p.y >= -margin &&
           p.y < height_ + margin;
}

bool Projection::below_horizon(double x, double y) const {
    // The up component of the (unnormalised) ray through the pixel.
    const double sx = (x - cx_) / focal_px_;
    const double sy = (cy_ - y) / focal_px_;
    return forward_[2] + sx * right_[2] + sy * up_[2] < 0.0;
}

Camera offset_camera(const Camera& camera, double right_deg, double up_deg) {
    double f[3], r[3], u[3];
    camera_basis(camera, f, r, u);
    const double tr = std::tan(right_deg * kDeg);
    const double tu = std::tan(up_deg * kDeg);
    double nf[3];
    for (int i = 0; i < 3; ++i) nf[i] = f[i] + tr * r[i] + tu * u[i];
    const double len = std::sqrt(dot(nf, nf));
    for (double& c : nf) c /= len;

    Camera out = camera;
    out.altitude_deg = std::asin(std::fmax(-1.0, std::fmin(1.0, nf[2]))) / kDeg;
    const double horizontal = std::hypot(nf[0], nf[1]);
    if (horizontal > 1e-9) {
        out.azimuth_deg = std::atan2(nf[0], nf[1]) / kDeg;
        if (out.azimuth_deg < 0.0) out.azimuth_deg += 360.0;
    }
    // Roll: the screen up of the old camera, seen from the new direction.
    double nu[3];
    const double along = dot(u, nf);
    for (int i = 0; i < 3; ++i) nu[i] = u[i] - along * nf[i];
    Camera level = out;
    level.roll_deg = 0.0;
    double lf[3], lr[3], lu[3];
    camera_basis(level, lf, lr, lu);
    out.roll_deg = std::atan2(-dot(nu, lr), dot(nu, lu)) / kDeg;
    return out;
}

Guidance guide(const Camera& camera, double target_azimuth_deg, double target_altitude_deg) {
    double f[3], r[3], u[3], v[3];
    camera_basis(camera, f, r, u);
    enu_from_horizontal(target_azimuth_deg, target_altitude_deg, v);
    Guidance g;
    g.separation_deg = std::acos(std::fmax(-1.0, std::fmin(1.0, dot(v, f)))) / kDeg;
    g.screen_bearing_deg = std::atan2(dot(v, r), dot(v, u)) / kDeg;
    if (g.screen_bearing_deg < 0.0) g.screen_bearing_deg += 360.0;
    return g;
}

} // namespace zerosky::sky
