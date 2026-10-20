/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <chrono>

namespace zerosky::orientation {

class MagneticGate {
public:
    enum class Status { Unknown, Healthy, Suspect, Disturbed };

    void reset();
    Status update(double measured_uT, double expected_uT,
                  std::chrono::steady_clock::time_point at);
    Status status() const { return status_; }
    bool allow_magnetic_correction() const { return status_ == Status::Healthy; }

private:
    Status status_ = Status::Unknown;
    std::chrono::steady_clock::time_point suspect_since_{};
    std::chrono::steady_clock::time_point recovering_since_{};
};

} // namespace zerosky::orientation
