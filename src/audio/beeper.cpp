/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "beeper.h"

#include <algorithm>
#include <cmath>

#if defined(ZEROSKY_BEEPER_SDL) && defined(ZEROSKY_BEEPER_ALSA)
#error "Choose one audio backend"
#endif

#if defined(ZEROSKY_BEEPER_SDL) || defined(ZEROSKY_BEEPER_ALSA)
#include <chrono>
#include <condition_variable>
#include <cerrno>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>

#ifdef ZEROSKY_BEEPER_SDL
#include <SDL2/SDL.h>
#else
#include <alsa/asoundlib.h>
#endif
#endif

namespace zerosky::audio {

GuidanceTone guidance_tone(double separation_deg) {
    if (!std::isfinite(separation_deg) || separation_deg < 0.0) return {};
    if (separation_deg < 1.0) return {true, true, 1600.0, 0, 0};

    // Equal distance ratios produce equal changes in cadence and pitch.
    const double limited = std::clamp(separation_deg, 1.0, 90.0);
    const double proximity = 1.0 - std::log(limited) / std::log(90.0);
    const double hz = 400.0 + 1200.0 * proximity;
    const int interval = static_cast<int>(std::lround(900.0 - 780.0 * proximity));
    // Variometer style: clear beeps that take about 40% of each period.
    const int pulse = std::clamp(static_cast<int>(std::lround(interval * 0.4)), 50, 150);
    return {true, false, hz, interval, pulse};
}

std::array<NotificationTone, 2> notification_tones(bool success) {
    return success ? std::array<NotificationTone, 2>{{{660.0, 110, 45}, {990.0, 160, 0}}}
                   : std::array<NotificationTone, 2>{{{220.0, 240, 0}, {0.0, 0, 0}}};
}

#if defined(ZEROSKY_BEEPER_SDL) || defined(ZEROSKY_BEEPER_ALSA)
namespace {
constexpr int kSampleRate = 48000;
constexpr double kPi = 3.14159265358979323846;
constexpr float kAmplitude = 0.12f;

void make_wave(std::vector<float>& samples, double hz, int duration_ms,
               bool continuous, double& phase) {
    const int count = kSampleRate * duration_ms / 1000;
    const int fade = kSampleRate * 3 / 1000;
    samples.resize(static_cast<size_t>(count));
    const double step = 2.0 * kPi * hz / kSampleRate;
    for (int i = 0; i < count; ++i) {
        float envelope = 1.0f;
        if (!continuous) {
            envelope = std::min({1.0f, static_cast<float>(i) / fade,
                                 static_cast<float>(count - 1 - i) / fade});
        }
        samples[static_cast<size_t>(i)] = kAmplitude * envelope * std::sin(phase);
        phase += step;
        if (phase >= 2.0 * kPi) phase -= 2.0 * kPi;
    }
}
} // namespace
#endif

struct Beeper::Impl {
#if defined(ZEROSKY_BEEPER_SDL) || defined(ZEROSKY_BEEPER_ALSA)
    std::mutex mutex;
    std::condition_variable wake;
    GuidanceTone tone;
    int notification{0}; // 1 success, -1 failure; worker consumes it once.
    bool shutting_down{false};
    std::thread worker;

#ifdef ZEROSKY_BEEPER_SDL
    SDL_AudioDeviceID device{0};
    bool sdl_initialized{false};
#else
    snd_pcm_t* device{nullptr};
#endif

    bool open_audio() {
#ifdef ZEROSKY_BEEPER_SDL
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;
        sdl_initialized = true;
        SDL_AudioSpec wanted{}, actual{};
        wanted.freq = kSampleRate;
        wanted.format = AUDIO_F32SYS;
        wanted.channels = 1;
        wanted.samples = 1024;
        wanted.callback = nullptr; // queued audio, filled by worker
        device = SDL_OpenAudioDevice(nullptr, 0, &wanted, &actual, 0);
        if (!device) return false;
        SDL_PauseAudioDevice(device, 0);
#else
        // The CardputerZero image runs PipeWire, which holds the ES8388 codec
        // exclusively: plughw:1,0 then fails with "Device busy". The pipewire
        // PCM from pipewire-alsa (a package dependency) goes through the
        // session daemon, as ZeroRadio does; plughw:1,0 covers images without it.
        const char* env = std::getenv("ZEROSKY_ALSA_DEVICE");
        const char* candidates[] = {env && env[0] ? env : "pipewire", "plughw:1,0"};
        for (const char* name : candidates) {
            if (snd_pcm_open(&device, name, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
                device = nullptr;
                continue;
            }
            if (snd_pcm_set_params(device, SND_PCM_FORMAT_FLOAT_LE,
                                   SND_PCM_ACCESS_RW_INTERLEAVED, 1, kSampleRate, 1, 100000) < 0 ||
                snd_pcm_nonblock(device, 1) < 0) {
                snd_pcm_close(device);
                device = nullptr;
                continue;
            }
            break;
        }
        if (!device) return false;
#endif
        return true;
    }

    void clear_audio() {
#ifdef ZEROSKY_BEEPER_SDL
        SDL_ClearQueuedAudio(device);
#else
        snd_pcm_drop(device);
        snd_pcm_prepare(device);
#endif
    }

    int queued_ms() {
#ifdef ZEROSKY_BEEPER_SDL
        return static_cast<int>(SDL_GetQueuedAudioSize(device) * 1000 /
                                (kSampleRate * sizeof(float)));
#else
        snd_pcm_sframes_t frames = 0;
        if (snd_pcm_delay(device, &frames) < 0 || frames < 0) return 0;
        return static_cast<int>(frames * 1000 / kSampleRate);
#endif
    }

    void submit(const std::vector<float>& samples) {
#ifdef ZEROSKY_BEEPER_SDL
        // A delayed UI frame must not leave a backlog of old guidance tones.
        if (SDL_GetQueuedAudioSize(device) > kSampleRate * sizeof(float) / 5)
            SDL_ClearQueuedAudio(device);
        SDL_QueueAudio(device, samples.data(),
                       static_cast<Uint32>(samples.size() * sizeof(float)));
#else
        // A beep can be longer than the 100 ms PCM buffer: write it in parts,
        // waiting for room, and recover from an underrun without dropping the
        // rest. The wait is bounded so a stuck device cannot hang the worker.
        const float* next = samples.data();
        size_t remaining = samples.size();
        for (int attempts = 0; remaining && attempts < 50; ++attempts) {
            const snd_pcm_sframes_t written = snd_pcm_writei(device, next, remaining);
            if (written > 0) {
                next += written;
                remaining -= static_cast<size_t>(written);
                continue;
            }
            if (written == -EAGAIN || written == 0) {
                snd_pcm_wait(device, 20);
                continue;
            }
            if (snd_pcm_recover(device, static_cast<int>(written), 1) < 0) break;
        }
#endif
    }

#ifdef ZEROSKY_BEEPER_ALSA
    // Notifications are short and are submitted only once. A nonblocking
    // write may accept just part of a note; the following gap can then put
    // the PCM into XRUN before the next note. Write and drain each note in
    // blocking mode so both notes actually reach the speaker.
    void submit_notification(const std::vector<float>& samples) {
        if (snd_pcm_nonblock(device, 0) < 0) return;
        const float* next = samples.data();
        size_t remaining = samples.size();
        while (remaining) {
            const snd_pcm_sframes_t written = snd_pcm_writei(device, next, remaining);
            if (written == -EINTR) continue;
            if (written < 0) {
                if (snd_pcm_recover(device, static_cast<int>(written), 0) < 0) break;
                continue;
            }
            if (written == 0) break;
            next += written;
            remaining -= static_cast<size_t>(written);
        }
        if (!remaining) snd_pcm_drain(device);
        snd_pcm_nonblock(device, 1);
        snd_pcm_prepare(device);
    }
#endif

    void run() {
        using clock = std::chrono::steady_clock;
        std::unique_lock<std::mutex> lock(mutex);
        bool was_active = false;
        bool was_continuous = false;
        auto last_pulse = clock::now() - std::chrono::milliseconds(900);
        double phase = 0.0;
        std::vector<float> samples;
        std::vector<float> silence;

        while (!shutting_down) {
            if (notification != 0) {
                const bool success = notification > 0;
                notification = 0;
                lock.unlock();
                clear_audio();
                const auto tones = notification_tones(success);
                for (const auto& note : tones) {
                    if (note.duration_ms == 0) break;
                    make_wave(samples, note.frequency_hz, note.duration_ms, false, phase);
#ifdef ZEROSKY_BEEPER_ALSA
                    submit_notification(samples);
                    std::this_thread::sleep_for(std::chrono::milliseconds(note.gap_ms));
#else
                    submit(samples);
                    std::this_thread::sleep_for(std::chrono::milliseconds(note.duration_ms + note.gap_ms));
#endif
                }
                lock.lock();
                was_active = false;
                continue;
            }
            if (!tone.enabled) {
                if (was_active) {
                    lock.unlock();
                    clear_audio();
                    lock.lock();
                    was_active = false;
                }
                wake.wait(lock, [this] { return shutting_down || tone.enabled || notification != 0; });
                continue;
            }

            const GuidanceTone current = tone;
            if (!was_active || was_continuous != current.continuous) {
                lock.unlock();
                clear_audio();
                lock.lock();
                last_pulse = clock::now() - std::chrono::milliseconds(900);
                phase = 0.0;
                if (shutting_down || !tone.enabled || tone.continuous != current.continuous)
                    continue;
            }
            was_active = true;
            was_continuous = current.continuous;

            if (current.continuous) {
                lock.unlock();
                const int queued = queued_ms();
                lock.lock();
                if (shutting_down || !tone.enabled || !tone.continuous) continue;
                if (queued > 80) {
                    wake.wait_for(lock, std::chrono::milliseconds(15));
                    continue;
                }
            } else {
                const auto due = last_pulse + std::chrono::milliseconds(current.interval_ms);
                if (clock::now() < due) {
                    // Keep the stream fed with silence between pulses. An empty
                    // stream underruns, and through PipeWire the recovery chops
                    // the next pulse into a warble.
                    lock.unlock();
                    if (queued_ms() < 40) {
                        silence.assign(static_cast<size_t>(kSampleRate * 20 / 1000), 0.0f);
                        submit(silence);
                    }
                    lock.lock();
                    wake.wait_until(lock, std::min(due, clock::now() + std::chrono::milliseconds(10)));
                    continue; // recalculate deadline after every distance update
                }
                last_pulse = clock::now();
            }

            lock.unlock();
            make_wave(samples, current.frequency_hz, current.continuous ? 60 : current.pulse_ms,
                      current.continuous, phase);
            submit(samples);
            lock.lock();
            if (current.continuous) {
                wake.wait_for(lock, std::chrono::milliseconds(30),
                              [this] { return shutting_down || !tone.enabled || !tone.continuous; });
            }
        }
        lock.unlock();
        clear_audio();
    }

    Impl() {
        if (open_audio()) worker = std::thread(&Impl::run, this);
    }

    ~Impl() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            shutting_down = true;
        }
        wake.notify_one();
        if (worker.joinable()) worker.join();
#ifdef ZEROSKY_BEEPER_SDL
        if (device) SDL_CloseAudioDevice(device);
        if (sdl_initialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
#else
        if (device) snd_pcm_close(device);
#endif
    }

    void set(GuidanceTone next) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            tone = next;
        }
        wake.notify_one();
    }

    void notify(bool success) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            notification = success ? 1 : -1;
        }
        wake.notify_one();
    }
#else
    void set(GuidanceTone) {}
    void notify(bool) {}
#endif
};

Beeper::Beeper() : impl_(std::make_unique<Impl>()) {}
Beeper::~Beeper() = default;

void Beeper::set_guidance(double separation_deg) {
    impl_->set(guidance_tone(separation_deg));
}

void Beeper::stop() {
    impl_->set({});
}

void Beeper::play_notification(bool success) {
    impl_->notify(success);
}

} // namespace zerosky::audio
