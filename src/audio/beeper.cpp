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
    return {true, false, hz, interval, 40};
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
        const char* name = std::getenv("ZEROSKY_ALSA_DEVICE");
        if (!name || !name[0]) name = "plughw:1,0"; // CardputerZero ES8388
        if (snd_pcm_open(&device, name, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
            device = nullptr;
            return false;
        }
        if (snd_pcm_set_params(device, SND_PCM_FORMAT_FLOAT_LE,
                               SND_PCM_ACCESS_RW_INTERLEAVED, 1, kSampleRate, 1, 100000) < 0 ||
            snd_pcm_nonblock(device, 1) < 0) {
            snd_pcm_close(device);
            device = nullptr;
            return false;
        }
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
        const snd_pcm_sframes_t written = snd_pcm_writei(device, samples.data(), samples.size());
        if (written < 0 && written != -EAGAIN) snd_pcm_recover(device, static_cast<int>(written), 1);
#endif
    }

    void run() {
        using clock = std::chrono::steady_clock;
        std::unique_lock<std::mutex> lock(mutex);
        bool was_active = false;
        bool was_continuous = false;
        auto last_pulse = clock::now() - std::chrono::milliseconds(900);
        double phase = 0.0;
        std::vector<float> samples;

        while (!shutting_down) {
            if (!tone.enabled) {
                if (was_active) {
                    lock.unlock();
                    clear_audio();
                    lock.lock();
                    was_active = false;
                }
                wake.wait(lock, [this] { return shutting_down || tone.enabled; });
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
                    wake.wait_until(lock, due);
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
#else
    void set(GuidanceTone) {}
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

} // namespace zerosky::audio
