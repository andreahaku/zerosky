/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "sensor_source.h"

#include <optional>

namespace zerosky::orientation {

using sensors::Vec3;

struct Quaternion {
    double w = 1.0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

// All sensor vectors passed here must already be rotated into the device body
// frame. The quaternion maps body vectors into the ENU world frame.
class Mahony {
public:
    struct Gains {
        double kp_accel = 2.0;
        double kp_magnetic = 1.0;
        double ki = 0.05;
    };

    Mahony();
    explicit Mahony(Gains gains);

    void reset(Quaternion body_to_enu = {});
    // Returns false for invalid inputs or a nonpositive/oversized time step.
    // A missing or rejected magnetometer leaves yaw to gyro integration.
    bool update(Vec3 gyro_radps, Vec3 accel_mps2,
                std::optional<Vec3> magnetic_body_uT,
                std::optional<Vec3> magnetic_enu_uT, double dt_seconds);

    Quaternion body_to_enu() const { return q_; }
    Vec3 gyro_bias_correction_radps() const { return gyro_bias_correction_; }
    bool used_accelerometer() const { return used_accelerometer_; }
    bool used_magnetometer() const { return used_magnetometer_; }

    static Vec3 rotate(Quaternion body_to_enu, Vec3 body_vector);
    static Vec3 rotate_inverse(Quaternion body_to_enu, Vec3 enu_vector);

private:
    Gains gains_;
    Quaternion q_;
    Vec3 gyro_bias_correction_;
    bool used_accelerometer_ = false;
    bool used_magnetometer_ = false;
};

} // namespace zerosky::orientation
