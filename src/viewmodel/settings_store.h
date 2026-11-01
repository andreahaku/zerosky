/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <filesystem>

namespace zerosky {

enum class ConstellationDisplay { Off = 0, Lines = 1, Names = 2 };

// The view choices kept between runs, in ~/.config/zerosky/settings.
struct ViewSettings {
    bool night_theme{true};
    bool daylight{true};
    bool camera_on{false};
    ConstellationDisplay constellations{ConstellationDisplay::Names};
    bool deep_sky{true};
    double zoom_fov_deg{60.0};
};

// How the camera image lines up with the sky, in ~/.config/zerosky/camera-alignment:
// the lens field of view and the offsets of the optical axis from the IMU
// pointing direction, in the screen frame (right, up), found by matching drawn
// stars to photographed ones.
struct CameraAlignment {
    double fov_deg{62.2};           // until measured: the Pi Camera v2 value, same IMX219
    double right_offset_deg{0.0};
    double up_offset_deg{0.0};
    bool measured{false};
};

// Both files are "key=value" lines. A missing file, an unknown key or a bad
// value leaves that field at its default. Writes are atomic: a temporary file,
// then a rename, so a crash never leaves half a file.
ViewSettings load_view_settings(const std::filesystem::path& file);
bool save_view_settings(const std::filesystem::path& file, const ViewSettings& settings);
CameraAlignment load_camera_alignment(const std::filesystem::path& file);
bool save_camera_alignment(const std::filesystem::path& file, const CameraAlignment& alignment);

} // namespace zerosky
