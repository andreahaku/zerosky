/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "beeper.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>

using namespace zerosky::audio;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
} // namespace

int main() {
    try {
        require(!guidance_tone(-1.0).enabled, "negative separation should be silent");
        require(!guidance_tone(std::numeric_limits<double>::quiet_NaN()).enabled,
                "NaN separation should be silent");
        require(!guidance_tone(std::numeric_limits<double>::infinity()).enabled,
                "infinite separation should be silent");

        const auto far = guidance_tone(90.0);
        require(far.enabled && !far.continuous && far.frequency_hz == 400.0 &&
                    far.interval_ms == 900 && far.pulse_ms == 40, "far endpoint wrong");
        const auto near = guidance_tone(1.0);
        require(near.enabled && !near.continuous && near.frequency_hz == 1600.0 &&
                    near.interval_ms == 120, "near endpoint wrong");
        require(guidance_tone(180.0).frequency_hz == far.frequency_hz,
                "wide separation should clamp");

        auto prior = far;
        for (double angle : {45.0, 15.0, 5.0, 1.0}) {
            const auto tone = guidance_tone(angle);
            require(tone.frequency_hz > prior.frequency_hz, "pitch must rise toward target");
            require(tone.interval_ms < prior.interval_ms, "pulses must accelerate toward target");
            require(tone.pulse_ms == 40, "pulse length changed");
            prior = tone;
        }

        for (double angle : {0.99, 0.1, 0.0}) {
            const auto tone = guidance_tone(angle);
            require(tone.enabled && tone.continuous && tone.frequency_hz == 1600.0,
                    "center should have continuous high tone");
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("beeper_test: ok");
    return 0;
}
