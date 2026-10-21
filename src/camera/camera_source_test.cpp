/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "camera_source.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

using namespace zerosky::camera;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool write_all(const uint8_t* data, size_t bytes) {
    while (bytes) {
        const ssize_t count = ::write(STDOUT_FILENO, data, bytes);
        if (count <= 0) return false;
        data += count;
        bytes -= static_cast<size_t>(count);
    }
    return true;
}

int fake_camera() {
    constexpr int height = 176;
    const size_t y_bytes = static_cast<size_t>(kFrameWidth) * height;
    std::vector<uint8_t> yuv(y_bytes * 3 / 2, 128);
    std::fill(yuv.begin(), yuv.begin() + y_bytes, 16);
    yuv[3 * kFrameWidth] = 200; // First pixel after centre crop.
    if (!write_all(yuv.data(), yuv.size())) return 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::fill(yuv.begin(), yuv.begin() + y_bytes, 235);
    return write_all(yuv.data(), yuv.size()) ? 0 : 1;
}

bool wait_for_sequence(CameraSource& source, CameraFrame& frame, uint64_t sequence,
                       std::chrono::milliseconds limit) {
    const auto deadline = std::chrono::steady_clock::now() + limit;
    do {
        if (source.copy_latest(frame) && frame.sequence >= sequence) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

} // namespace

int main(int argc, char** argv) {
    // RpicamSource invokes this executable with rpicam-vid's arguments. No
    // camera hardware is needed for the pipe, framing and conversion test.
    if (argc > 1 && std::string(argv[1]) == "--nopreview") return fake_camera();

    std::string error;
    CameraFrame frame;
    TestPatternSource pattern;
    require(pattern.start(error), error.c_str());
    require(pattern.copy_latest(frame), "Pattern source produced no initial frame");
    require(frame.rgb565.size() == static_cast<size_t>(kFrameWidth) * kFrameHeight,
            "Pattern dimensions are wrong");
    require(frame.captured_at != std::chrono::steady_clock::time_point{},
            "Pattern timestamp missing");
    require(wait_for_sequence(pattern, frame, 2, std::chrono::milliseconds(300)),
            "Pattern did not advance");
    pattern.stop();

    RpicamOptions options;
    options.executable = argv[0];
    options.retry_delay_ms = 2000;
    RpicamSource pipe(options);
    require(pipe.start(error), error.c_str());
    require(wait_for_sequence(pipe, frame, 1, std::chrono::seconds(1)),
            "Pipe did not publish first complete YUV frame");
    require(frame.rgb565.size() == static_cast<size_t>(kFrameWidth) * kFrameHeight,
            "Pipe dimensions are wrong");
    require(frame.rgb565.front() == 0xce59,
            "Centre crop or first YUV pixel conversion is wrong");
    require(frame.rgb565[1] == 0x1082, "Neutral YUV colour is wrong");
    require(wait_for_sequence(pipe, frame, 2, std::chrono::seconds(1)),
            "Pipe did not publish second YUV frame");
    require(frame.rgb565.front() == 0xef5d, "Second YUV frame is wrong");
    pipe.stop();
    require(pipe.start(error), error.c_str());
    require(wait_for_sequence(pipe, frame, 1, std::chrono::seconds(1)),
            "Camera source did not restart after a toggle");
    pipe.stop();

    RpicamOptions missing;
    missing.executable = "/nonexistent/zerosky-rpicam-vid";
    RpicamSource absent(missing);
    require(absent.start(error), "Missing camera helper crashed at start");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    require(!absent.copy_latest(frame), "Missing camera published an image");
    require(!absent.last_error().empty(), "Missing camera did not report an error");
    absent.stop();
}
