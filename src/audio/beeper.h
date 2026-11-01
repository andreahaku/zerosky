/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <array>
#include <memory>

namespace zerosky::audio {

struct GuidanceTone {
    bool enabled{false};
    bool continuous{false};
    double frequency_hz{0.0};
    int interval_ms{0}; // start-to-start interval for pulses
    int pulse_ms{0};
};

// Pure distance-to-sound map. Negative or non-finite distances mean silence.
GuidanceTone guidance_tone(double separation_deg);

struct NotificationTone {
    double frequency_hz{0.0};
    int duration_ms{0};
    int gap_ms{0};
};

// Success is a rising pair; failure is one low tone.
std::array<NotificationTone, 2> notification_tones(bool success);

// Optional audio output. When SDL/ALSA is unavailable, calls are safe no-ops.
// Define ZEROSKY_BEEPER_SDL or ZEROSKY_BEEPER_ALSA when building the backend.
class Beeper {
public:
    Beeper();
    ~Beeper();
    Beeper(const Beeper&) = delete;
    Beeper& operator=(const Beeper&) = delete;

    void set_guidance(double separation_deg);
    void play_notification(bool success);
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace zerosky::audio
