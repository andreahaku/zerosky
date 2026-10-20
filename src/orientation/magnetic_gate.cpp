/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "magnetic_gate.h"

#include <algorithm>
#include <cmath>

namespace zerosky::orientation {

void MagneticGate::reset() {
    status_ = Status::Unknown;
    suspect_since_ = {};
    recovering_since_ = {};
}

MagneticGate::Status MagneticGate::update(
    double measured_uT, double expected_uT,
    std::chrono::steady_clock::time_point at) {
    if (!std::isfinite(measured_uT) || !std::isfinite(expected_uT) ||
        measured_uT <= 0.0 || expected_uT <= 0.0 ||
        at == std::chrono::steady_clock::time_point{}) {
        reset();
        return status_;
    }
    const double delta = std::abs(measured_uT - expected_uT);
    const double enter = std::max(8.0, 0.20 * expected_uT);
    const double leave = std::max(5.0, 0.12 * expected_uT);

    if (status_ == Status::Disturbed) {
        if (delta <= leave) {
            if (recovering_since_ == std::chrono::steady_clock::time_point{})
                recovering_since_ = at;
            else if (at >= recovering_since_ + std::chrono::seconds(3)) {
                status_ = Status::Healthy;
                suspect_since_ = {};
                recovering_since_ = {};
            }
        } else {
            recovering_since_ = {};
        }
        return status_;
    }

    if (delta > enter) {
        if (suspect_since_ == std::chrono::steady_clock::time_point{})
            suspect_since_ = at;
        status_ = at >= suspect_since_ + std::chrono::seconds(1)
                  ? Status::Disturbed : Status::Suspect;
        recovering_since_ = {};
    } else {
        status_ = Status::Healthy;
        suspect_since_ = {};
    }
    return status_;
}

} // namespace zerosky::orientation
