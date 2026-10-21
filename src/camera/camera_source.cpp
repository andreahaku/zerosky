/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "camera_source.h"

#include "platform/subprocess.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <exception>
#include <utility>

namespace zerosky::camera {
namespace {

constexpr int kCaptureHeight = 176; // Multiple of 16, with a 3-pixel crop per side.
constexpr size_t kYuvBytes = static_cast<size_t>(kFrameWidth) * kCaptureHeight * 3 / 2;

uint16_t rgb565(int r, int g, int b) {
    const auto channel = [](int value) { return std::clamp(value, 0, 255); };
    r = channel(r);
    g = channel(g);
    b = channel(b);
    return static_cast<uint16_t>(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}

std::vector<uint16_t> convert_yuv420(const uint8_t* yuv) {
    std::vector<uint16_t> out(static_cast<size_t>(kFrameWidth) * kFrameHeight);
    const size_t luma_size = static_cast<size_t>(kFrameWidth) * kCaptureHeight;
    const uint8_t* u_plane = yuv + luma_size;
    const uint8_t* v_plane = u_plane + luma_size / 4;
    for (int y = 0; y < kFrameHeight; ++y) {
        const int source_y = y + (kCaptureHeight - kFrameHeight) / 2;
        for (int x = 0; x < kFrameWidth; ++x) {
            const int luma = yuv[source_y * kFrameWidth + x];
            const int uv_index = (source_y / 2) * (kFrameWidth / 2) + x / 2;
            const int u = static_cast<int>(u_plane[uv_index]) - 128;
            const int v = static_cast<int>(v_plane[uv_index]) - 128;
            // rpicam-vid's raw YUV420 output uses JPEG/full-range BT.601.
            const int r = luma + ((359 * v) >> 8);
            const int g = luma - ((88 * u + 183 * v) >> 8);
            const int b = luma + ((454 * u) >> 8);
            out[static_cast<size_t>(y) * kFrameWidth + x] = rgb565(r, g, b);
        }
    }
    return out;
}

void interruptible_wait(const std::atomic<bool>& running, int milliseconds) {
    const auto until = std::chrono::steady_clock::now() +
                       std::chrono::milliseconds(milliseconds);
    while (running.load() && std::chrono::steady_clock::now() < until)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

} // namespace

bool CameraSource::copy_latest(CameraFrame& frame) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (latest_.sequence == 0) return false;
    frame = latest_;
    return true;
}

std::string CameraSource::last_error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error_;
}

void CameraSource::publish(std::vector<uint16_t> pixels,
                           std::chrono::steady_clock::time_point captured_at) {
    if (pixels.size() != static_cast<size_t>(kFrameWidth) * kFrameHeight) return;
    std::lock_guard<std::mutex> lock(mutex_);
    latest_.rgb565 = std::move(pixels);
    latest_.captured_at = captured_at;
    ++latest_.sequence;
    error_.clear();
}

void CameraSource::set_error(std::string error) {
    std::lock_guard<std::mutex> lock(mutex_);
    error_ = std::move(error);
}

void CameraSource::clear_frame() {
    std::lock_guard<std::mutex> lock(mutex_);
    latest_ = {};
    error_.clear();
}

TestPatternSource::~TestPatternSource() { stop(); }

bool TestPatternSource::start(std::string& error) {
    stop();
    clear_frame();
    error.clear();
    running_ = true;
    make_frame(0);
    try {
        worker_ = std::thread(&TestPatternSource::run, this);
    } catch (const std::exception& ex) {
        running_ = false;
        error = ex.what();
        set_error(error);
        return false;
    }
    return true;
}

void TestPatternSource::stop() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
}

void TestPatternSource::make_frame(uint64_t step) {
    std::vector<uint16_t> pixels(static_cast<size_t>(kFrameWidth) * kFrameHeight);
    const int marker = static_cast<int>((step * 7) % kFrameWidth);
    for (int y = 0; y < kFrameHeight; ++y) {
        for (int x = 0; x < kFrameWidth; ++x) {
            const bool crosshair = std::abs(x - marker) <= 1 ||
                                   (std::abs(y - kFrameHeight / 2) <= 1 &&
                                    std::abs(x - kFrameWidth / 2) < 12);
            pixels[static_cast<size_t>(y) * kFrameWidth + x] = crosshair
                ? rgb565(240, 48, 48)
                : rgb565(12 + x / 4, 12 + y / 2, 30 + x / 7);
        }
    }
    publish(std::move(pixels), std::chrono::steady_clock::now());
}

void TestPatternSource::run() {
    uint64_t step = 1;
    auto next = std::chrono::steady_clock::now();
    while (running_) {
        next += std::chrono::milliseconds(100);
        std::this_thread::sleep_until(next);
        if (!running_) break;
        make_frame(step++);
    }
}

RpicamSource::RpicamSource(RpicamOptions options) : options_(std::move(options)) {}
RpicamSource::~RpicamSource() { stop(); }

bool RpicamSource::start(std::string& error) {
    stop();
    clear_frame();
    error.clear();
    if (options_.executable.empty() || options_.frames_per_second < 5 ||
        options_.frames_per_second > 15 ||
        (options_.rotation_degrees != 0 && options_.rotation_degrees != 180) ||
        options_.retry_delay_ms < 100) {
        error = "Invalid rpicam-vid options";
        set_error(error);
        return false;
    }
    running_ = true;
    try {
        worker_ = std::thread(&RpicamSource::run, this);
    } catch (const std::exception& ex) {
        running_ = false;
        error = ex.what();
        set_error(error);
        return false;
    }
    return true;
}

void RpicamSource::stop() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
}

void RpicamSource::run() {
    const std::vector<std::string> argv{
        options_.executable, "--nopreview", "--timeout", "0", "--codec", "yuv420",
        "--width", std::to_string(kFrameWidth), "--height", std::to_string(kCaptureHeight),
        "--framerate", std::to_string(options_.frames_per_second),
        "--rotation", std::to_string(options_.rotation_degrees), "--flush", "--output", "-"};
    std::array<uint8_t, kYuvBytes> raw{};
    while (running_) {
        toolkit::Subprocess child;
        if (!child.start(argv)) {
            set_error("Cannot start rpicam-vid");
            interruptible_wait(running_, options_.retry_delay_ms);
            continue;
        }
        size_t filled = 0;
        auto last_complete_frame = std::chrono::steady_clock::now();
        bool have_frame = false;
        while (running_) {
            const ssize_t count = child.read_stdout(
                reinterpret_cast<char*>(raw.data() + filled), raw.size() - filled);
            if (count < 0) {
                set_error("Camera stream ended or rpicam-vid is unavailable");
                break;
            }
            const auto now = std::chrono::steady_clock::now();
            const auto timeout = have_frame ? std::chrono::seconds(3)
                                            : std::chrono::seconds(8);
            if (now - last_complete_frame > timeout) {
                set_error("Camera stream timed out before a complete frame");
                break;
            }
            if (count == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            filled += static_cast<size_t>(count);
            if (filled == raw.size()) {
                publish(convert_yuv420(raw.data()), now);
                have_frame = true;
                last_complete_frame = now;
                filled = 0;
            }
        }
        child.stop();
        if (running_) interruptible_wait(running_, options_.retry_delay_ms);
    }
}

} // namespace zerosky::camera
