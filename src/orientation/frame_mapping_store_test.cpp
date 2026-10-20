/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "frame_mapping_store.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace zerosky::orientation;

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string read_text(const std::filesystem::path& file) {
    std::ifstream input(file);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_text(const std::filesystem::path& file, const std::string& content) {
    std::ofstream output(file);
    output << content;
    require(output.good(), "Cannot write fixture");
}

std::string replace_once(std::string text, const std::string& old_value,
                         const std::string& new_value) {
    const auto at = text.find(old_value);
    require(at != std::string::npos, "Test fixture token missing");
    text.replace(at, old_value.size(), new_value);
    return text;
}

FrameMapping measured() {
    FrameMapping mapping;
    mapping.gyro_to_body = {{{0, -1, 0}, {1, 0, 0}, {0, 0, 1}}};
    mapping.pointing_body = {0, 1, 0};
    mapping.screen_up_body = {0, 0, 1};
    mapping.verified = true;
    return mapping;
}

void round_trip_and_backup(const std::filesystem::path& file) {
    std::string error;
    FrameMapping loaded;
    loaded.verified = true;
    require(!load_frame_mapping(file, loaded, error) && !loaded.verified,
            "Missing file kept verified state");

    const FrameMapping first = measured();
    require(save_frame_mapping(file, first, error), error.c_str());
    struct stat mode{};
    require(::stat(file.c_str(), &mode) == 0 && (mode.st_mode & 0777) == 0600,
            "Frame mapping permissions are not private");
    require(load_frame_mapping(file, loaded, error) && loaded.verified, error.c_str());
    require(loaded.gyro_to_body[0][1] == -1 && loaded.pointing_body.y == 1 &&
                loaded.screen_up_body.z == 1, "Loaded frame differs");
    const std::string original = read_text(file);

    FrameMapping second = first;
    second.pointing_body = {1, 0, 0};
    second.screen_up_body = {0, 0, 1};
    require(save_frame_mapping(file, second, error), error.c_str());
    require(read_text(file.string() + ".bak") == original, "Backup does not contain prior frame");
    require(load_frame_mapping(file, loaded, error) && loaded.pointing_body.x == 1,
            "Second frame not loaded");

    const std::string before_failure = read_text(file);
    second.verified = false;
    require(!save_frame_mapping(file, second, error), "Saved unverified frame");
    second.verified = true;
    second.accel_to_body[0][0] = 1.2;
    require(!save_frame_mapping(file, second, error), "Saved non-orthonormal matrix");
    second = first;
    second.screen_up_body = second.pointing_body;
    require(!save_frame_mapping(file, second, error), "Saved parallel axes");
    second = first;
    second.pointing_body.y = 1.1;
    require(!save_frame_mapping(file, second, error), "Saved non-unit axis");
    require(read_text(file) == before_failure, "Rejected save changed frame file");
}

void reject_bad_files(const std::filesystem::path& file) {
    const std::string good = read_text(file);
    const std::string matrix = "[[1,0,0],[0,1,0],[0,0,1]]";
    const std::string pointing = "\"pointing_body\":[1,0,0]";
    const std::string up = "\"screen_up_body\":[0,0,1]";
    const std::string bad[] = {
        replace_once(good, "\"schema\":1", "\"schema\":2"),
        replace_once(good, "\"schema\":1,", ""),
        replace_once(good, "\"schema\":1,", "\"schema\":1,\"schema\":1,"),
        replace_once(good, "\"schema\":1,", "\"schema\":1,\"extra\":0,"),
        replace_once(good, matrix, "[[2,0,0],[0,1,0],[0,0,1]]"),
        replace_once(good, pointing, "\"pointing_body\":[2,0,0]"),
        replace_once(good, up, "\"screen_up_body\":[1,0,0]"),
        replace_once(good, pointing, "\"pointing_body\":[NaN,0,0]"),
        good + "garbage",
        std::string(4097, ' ')
    };
    for (const auto& text : bad) {
        write_text(file, text);
        FrameMapping loaded = measured();
        std::string error;
        require(!load_frame_mapping(file, loaded, error) && !loaded.verified,
                "Accepted invalid JSON or kept verified state");
        require(!save_frame_mapping(file, measured(), error) && read_text(file) == text,
                "Overwrote invalid existing frame");
    }
    write_text(file, good);
}

} // namespace

int main() {
    try {
        const auto directory = std::filesystem::temp_directory_path() /
                               ("zerosky_frame_mapping_test_" + std::to_string(::getpid()));
        std::filesystem::create_directories(directory);
        const auto file = directory / "frame-mapping.json";
        round_trip_and_backup(file);
        reject_bad_files(file);
        std::filesystem::remove_all(directory);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("frame_mapping_store_test: ok");
    return 0;
}
