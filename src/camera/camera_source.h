/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace zerosky::camera {

constexpr int kFrameWidth = 320;
constexpr int kFrameHeight = 170;

struct CameraFrame {
    std::vector<uint16_t> rgb565; // Row-major, 320 x 170, native LVGL RGB565.
    std::chrono::steady_clock::time_point captured_at{};
    uint64_t sequence = 0;
};

class CameraSource {
public:
    virtual ~CameraSource() = default;
    CameraSource(const CameraSource&) = delete;
    CameraSource& operator=(const CameraSource&) = delete;

    virtual bool start(std::string& error) = 0;
    virtual void stop() = 0;

    // Copies only a complete latest frame. The caller should discard a frame
    // whose captured_at is stale rather than leaving a frozen camera image.
    bool copy_latest(CameraFrame& frame) const;
    std::string last_error() const;

protected:
    CameraSource() = default;
    void publish(std::vector<uint16_t> rgb565,
                 std::chrono::steady_clock::time_point captured_at);
    void set_error(std::string error);
    void clear_frame();

private:
    mutable std::mutex mutex_;
    CameraFrame latest_;
    std::string error_;
};

// Deterministic moving pattern for the desktop simulator. No camera hardware
// or external process is touched.
class TestPatternSource final : public CameraSource {
public:
    ~TestPatternSource() override;
    bool start(std::string& error) override;
    void stop() override;

private:
    void run();
    void make_frame(uint64_t step);
    std::atomic<bool> running_{false};
    std::thread worker_;
};

struct RpicamOptions {
    std::string executable = "rpicam-vid";
    int frames_per_second = 10; // Supported range: 5-15.
    int rotation_degrees = 180;  // CameraApp rotates the same IMX219 preview.
    int retry_delay_ms = 2000;
};

// Device backend. rpicam-vid emits tightly packed, headerless YUV420 frames
// at 320 x 176. The worker owns the child process and performs conversion.
// start() starts the worker; camera or executable errors surface in last_error().
class RpicamSource final : public CameraSource {
public:
    explicit RpicamSource(RpicamOptions options = {});
    ~RpicamSource() override;
    bool start(std::string& error) override;
    void stop() override;

private:
    void run();
    RpicamOptions options_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace zerosky::camera
