/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "settings_store.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

using namespace zerosky;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(double a, double b) { return std::abs(a - b) < 1e-3; }

void round_trip(const std::filesystem::path& dir) {
    const auto settings_file = dir / "settings";
    ViewSettings saved;
    saved.night_theme = false;
    saved.daylight = false;
    saved.camera_on = true;
    saved.constellations = ConstellationDisplay::Lines;
    saved.deep_sky = false;
    saved.zoom_fov_deg = 33.5;
    require(save_view_settings(settings_file, saved), "save settings");
    const auto loaded = load_view_settings(settings_file);
    require(!loaded.night_theme && !loaded.daylight && loaded.camera_on &&
                loaded.constellations == ConstellationDisplay::Lines && !loaded.deep_sky,
            "settings flags");
    require(near(loaded.zoom_fov_deg, 33.5), "settings zoom");
    require(!std::filesystem::exists(dir / "settings.tmp"), "temporary file left behind");

    const auto alignment_file = dir / "camera-alignment";
    CameraAlignment alignment;
    alignment.fov_deg = 58.25;
    alignment.right_offset_deg = -1.4;
    alignment.up_offset_deg = 2.2;
    alignment.measured = true;
    require(save_camera_alignment(alignment_file, alignment), "save alignment");
    const auto back = load_camera_alignment(alignment_file);
    require(near(back.fov_deg, 58.25) && near(back.right_offset_deg, -1.4) &&
                near(back.up_offset_deg, 2.2) && back.measured,
            "alignment values");
}

void bad_values_keep_defaults(const std::filesystem::path& dir) {
    const auto file = dir / "bad";
    {
        std::ofstream out(file);
        out << "night_theme=yes\nzoom_fov_deg=500\ncamera_on=1\n"
            << "constellations=3\ndeep_sky=yes\nnonsense\nfov_deg=abc\n"
            << "right_offset_deg=nan\nup_offset_deg=3x\n";
    }
    const auto settings = load_view_settings(file);
    require(settings.night_theme && near(settings.zoom_fov_deg, 60.0) && settings.camera_on &&
                settings.constellations == ConstellationDisplay::Names && settings.deep_sky,
            "bad settings not ignored");
    const auto alignment = load_camera_alignment(file);
    require(near(alignment.fov_deg, 62.2) && near(alignment.right_offset_deg, 0.0) &&
                near(alignment.up_offset_deg, 0.0) && !alignment.measured,
            "bad alignment not ignored");
    // Blanks and Windows line ends, as from a hand edit, are accepted.
    const auto edited = dir / "edited";
    {
        std::ofstream out(edited);
        out << " night_theme = 0 \r\nzoom_fov_deg=45\r\n";
    }
    const auto from_edit = load_view_settings(edited);
    require(!from_edit.night_theme && near(from_edit.zoom_fov_deg, 45.0), "hand-edited file not read");
    const auto missing = load_view_settings(dir / "missing");
    require(missing.night_theme && missing.daylight && !missing.camera_on &&
                missing.constellations == ConstellationDisplay::Names && missing.deep_sky,
            "missing file defaults");
}

void failed_rename_is_reported(const std::filesystem::path& dir) {
    // The destination is a directory: the rename fails and so must the save.
    const auto blocked = dir / "blocked";
    std::filesystem::create_directories(blocked / "inside");
    require(!save_view_settings(blocked, ViewSettings{}), "failed save reported as success");
    require(!std::filesystem::exists(dir / "blocked.tmp"), "temporary file left after failure");
}

} // namespace

int main() {
    // One directory per run, so parallel runs do not share files.
    const auto dir = std::filesystem::temp_directory_path() /
                     ("zerosky_settings_store_test_" + std::to_string(::getpid()));
    std::filesystem::remove_all(dir);
    try {
        round_trip(dir);
        bad_values_keep_defaults(dir);
        failed_rename_is_reported(dir);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        std::filesystem::remove_all(dir);
        return 1;
    }
    std::filesystem::remove_all(dir);
    std::puts("settings_store_test: ok");
    return 0;
}
