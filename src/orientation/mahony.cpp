/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "mahony.h"

#include <cmath>

namespace zerosky::orientation {
namespace {

bool finite(Vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

double norm(Vec3 v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3 scaled(Vec3 v, double scale) { return {v.x * scale, v.y * scale, v.z * scale}; }
Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

Quaternion product(Quaternion a, Quaternion b) {
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

Quaternion normalized(Quaternion q) {
    const double magnitude = std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    if (!std::isfinite(magnitude) || magnitude < 1e-12) return {};
    return {q.w / magnitude, q.x / magnitude, q.y / magnitude, q.z / magnitude};
}

} // namespace

Mahony::Mahony() = default;
Mahony::Mahony(Gains gains) : gains_(gains) {}

void Mahony::reset(Quaternion body_to_enu) {
    q_ = normalized(body_to_enu);
    gyro_bias_correction_ = {};
    used_accelerometer_ = false;
    used_magnetometer_ = false;
}

Vec3 Mahony::rotate(Quaternion q, Vec3 body_vector) {
    q = normalized(q);
    const Quaternion result = product(product(q, {0.0, body_vector.x, body_vector.y,
                                                    body_vector.z}),
                                      {q.w, -q.x, -q.y, -q.z});
    return {result.x, result.y, result.z};
}

Vec3 Mahony::rotate_inverse(Quaternion q, Vec3 enu_vector) {
    return rotate({q.w, -q.x, -q.y, -q.z}, enu_vector);
}

bool Mahony::update(Vec3 gyro_radps, Vec3 accel_mps2,
                    std::optional<Vec3> magnetic_body_uT,
                    std::optional<Vec3> magnetic_enu_uT, double dt_seconds) {
    used_accelerometer_ = false;
    used_magnetometer_ = false;
    if (!finite(gyro_radps) || !finite(accel_mps2) ||
        !std::isfinite(dt_seconds) || dt_seconds <= 0.0 || dt_seconds > 0.25 ||
        !std::isfinite(gains_.kp_accel) || gains_.kp_accel < 0.0 ||
        !std::isfinite(gains_.kp_magnetic) || gains_.kp_magnetic < 0.0 ||
        !std::isfinite(gains_.ki) || gains_.ki < 0.0) return false;

    Vec3 correction{};
    const double accel_norm = norm(accel_mps2);
    constexpr double kGravity = 9.80665;
    const double accel_deviation = std::abs(accel_norm - kGravity) / kGravity;
    if (accel_norm > 1e-9 && accel_deviation < 0.30) {
        const double weight = accel_deviation < 0.10 ? 1.0 : (0.30 - accel_deviation) / 0.20;
        const Vec3 measured_up = scaled(accel_mps2, 1.0 / accel_norm);
        const Vec3 predicted_up = rotate_inverse(q_, {0.0, 0.0, 1.0});
        correction = add(correction,
                         scaled(cross(measured_up, predicted_up), gains_.kp_accel * weight));
        used_accelerometer_ = true;
    }

    if (magnetic_body_uT && magnetic_enu_uT &&
        finite(*magnetic_body_uT) && finite(*magnetic_enu_uT)) {
        const double measured_norm = norm(*magnetic_body_uT);
        const double expected_norm = norm(*magnetic_enu_uT);
        if (measured_norm > 1e-9 && expected_norm > 1e-9) {
            const Vec3 measured = scaled(*magnetic_body_uT, 1.0 / measured_norm);
            const Vec3 predicted = rotate_inverse(q_,
                                  scaled(*magnetic_enu_uT, 1.0 / expected_norm));
            correction = add(correction,
                             scaled(cross(measured, predicted), gains_.kp_magnetic));
            used_magnetometer_ = true;
        }
    }

    if (used_accelerometer_ || used_magnetometer_) {
        gyro_bias_correction_ = add(gyro_bias_correction_,
                                    scaled(correction, gains_.ki * dt_seconds));
        // Limit a bad sample's persistent effect to roughly 20 degrees/second.
        const double bias_norm = norm(gyro_bias_correction_);
        constexpr double kMaxBias = 0.35;
        if (bias_norm > kMaxBias)
            gyro_bias_correction_ = scaled(gyro_bias_correction_, kMaxBias / bias_norm);
    }
    const Vec3 corrected_gyro = add(add(gyro_radps, correction), gyro_bias_correction_);
    const Quaternion rate = product(q_, {0.0, corrected_gyro.x, corrected_gyro.y,
                                            corrected_gyro.z});
    q_ = normalized({q_.w + 0.5 * rate.w * dt_seconds,
                     q_.x + 0.5 * rate.x * dt_seconds,
                     q_.y + 0.5 * rate.y * dt_seconds,
                     q_.z + 0.5 * rate.z * dt_seconds});
    return true;
}

} // namespace zerosky::orientation
