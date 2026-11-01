/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace zerosky::sky {

// Where the screen looks: the azimuth and altitude of the screen centre, the
// roll of the screen around that direction (positive turns the image
// clockwise), and the horizontal field of view.
struct Camera {
    double azimuth_deg{0.0};
    double altitude_deg{0.0};
    double roll_deg{0.0};
    double fov_deg{60.0};
};

struct ScreenPoint {
    double x{0.0};
    double y{0.0};
    bool visible{false}; // in front of the camera; x/y may still fall off screen
};

// Gnomonic (pinhole) projection of sky directions onto a width x height screen,
// so straight lines stay straight, as through a camera lens.
class Projection {
public:
    Projection(const Camera& camera, int width, int height);

    ScreenPoint project(double azimuth_deg, double altitude_deg) const;

    // True when the point is in front of the camera and inside the screen,
    // grown by `margin` pixels on every side.
    bool on_screen(const ScreenPoint& p, double margin = 0.0) const;

    // True when the screen pixel (x, y) looks below the horizon.
    bool below_horizon(double x, double y) const;

private:
    double forward_[3];
    double right_[3];
    double up_[3];
    double focal_px_;
    double cx_;
    double cy_;
    int width_;
    int height_;
};

// The camera turned by small angles in its own frame: `right_deg` towards
// screen right, `up_deg` towards screen up. A misaligned lens is offset in the
// device frame, so the correction must turn with the roll.
Camera offset_camera(const Camera& camera, double right_deg, double up_deg);

// The unit vector of a direction in the east-north-up frame.
void enu_from_horizontal(double azimuth_deg, double altitude_deg, double out[3]);

// The shortest turn from the camera centre to a target: the angle between them
// and the screen direction to turn towards (0 = up, 90 = right), for the search arrow.
struct Guidance {
    double separation_deg{0.0};
    double screen_bearing_deg{0.0};
};
Guidance guide(const Camera& camera, double target_azimuth_deg, double target_altitude_deg);

} // namespace zerosky::sky
