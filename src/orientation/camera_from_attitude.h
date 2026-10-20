/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "attitude_service.h"
#include "../sky/projection.h"

#include <optional>

namespace zerosky::orientation {

// Converts the verified body frame to Projection's azimuth, altitude and roll.
// Retains the previous field of view. Near zenith/nadir, where azimuth is
// undefined, retains the previous azimuth and adjusts roll to keep screen-up
// continuous. Returns no camera for an unverified or degenerate frame.
std::optional<sky::Camera> camera_from_attitude(
    const AttitudeSnapshot& snapshot, const FrameMapping& mapping,
    const sky::Camera& previous_camera = {});

} // namespace zerosky::orientation
