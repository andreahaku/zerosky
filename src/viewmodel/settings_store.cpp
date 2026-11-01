/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "settings_store.h"

#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <system_error>

namespace zerosky {
namespace {

// Without leading and trailing blanks, \r included (a file edited on Windows).
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}

std::map<std::string, std::string> read_pairs(const std::filesystem::path& file) {
    std::map<std::string, std::string> pairs;
    std::ifstream in(file);
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        const auto eq = line.find('=');
        if (line.empty() || line[0] == '#' || eq == std::string::npos) continue;
        pairs[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
    }
    return pairs;
}

void read_bool(const std::map<std::string, std::string>& pairs, const char* key, bool& out) {
    const auto it = pairs.find(key);
    if (it == pairs.end()) return;
    if (it->second == "1") out = true;
    if (it->second == "0") out = false;
}

void read_constellations(const std::map<std::string, std::string>& pairs,
                         ConstellationDisplay& out) {
    const auto it = pairs.find("constellations");
    if (it == pairs.end()) return;
    if (it->second == "0") out = ConstellationDisplay::Off;
    if (it->second == "1") out = ConstellationDisplay::Lines;
    if (it->second == "2") out = ConstellationDisplay::Names;
}

// Keeps `out` when the value is missing, not a number, or outside [lo, hi].
void read_number(const std::map<std::string, std::string>& pairs, const char* key, double lo,
                 double hi, double& out) {
    const auto it = pairs.find(key);
    if (it == pairs.end() || it->second.empty()) return;
    char* end = nullptr;
    const double value = std::strtod(it->second.c_str(), &end);
    if (end != it->second.c_str() + it->second.size() || !std::isfinite(value)) return;
    if (value < lo || value > hi) return;
    out = value;
}

// fsync on a path (a file, or a directory to persist a rename in it).
bool sync_path(const std::filesystem::path& path, int flags) {
    const int fd = ::open(path.c_str(), flags);
    if (fd < 0) return false;
    const bool ok = ::fsync(fd) == 0;
    ::close(fd);
    return ok;
}

// Temporary file, fsync, rename, fsync of the directory: after a power cut the
// file holds either the old or the new settings, never half of them.
bool write_atomically(const std::filesystem::path& file, const std::string& text) {
    if (file.empty()) return false;
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    if (ec) return false;
    auto temp = file;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        out << text;
        out.flush();
        if (!out) return false;
    }
    if (!sync_path(temp, O_RDONLY)) {
        std::filesystem::remove(temp, ec);
        return false;
    }
    std::error_code rename_error;
    std::filesystem::rename(temp, file, rename_error);
    if (rename_error) {
        std::error_code cleanup_error; // keep the rename error: the save failed
        std::filesystem::remove(temp, cleanup_error);
        return false;
    }
    sync_path(file.parent_path(), O_RDONLY | O_DIRECTORY);
    return true;
}

std::string number(double value) {
    char text[32];
    std::snprintf(text, sizeof text, "%.3f", value);
    return text;
}

} // namespace

ViewSettings load_view_settings(const std::filesystem::path& file) {
    ViewSettings settings;
    const auto pairs = read_pairs(file);
    read_bool(pairs, "night_theme", settings.night_theme);
    read_bool(pairs, "daylight", settings.daylight);
    read_bool(pairs, "camera_on", settings.camera_on);
    read_constellations(pairs, settings.constellations);
    read_bool(pairs, "deep_sky", settings.deep_sky);
    read_number(pairs, "zoom_fov_deg", 15.0, 120.0, settings.zoom_fov_deg);
    return settings;
}

bool save_view_settings(const std::filesystem::path& file, const ViewSettings& settings) {
    return write_atomically(file, "# ZeroSky view settings\n"
                                  "night_theme=" + std::string(settings.night_theme ? "1" : "0") +
                                  "\ndaylight=" + (settings.daylight ? "1" : "0") +
                                  "\ncamera_on=" + (settings.camera_on ? "1" : "0") +
                                  "\nconstellations=" +
                                  std::to_string(static_cast<int>(settings.constellations)) +
                                  "\ndeep_sky=" + (settings.deep_sky ? "1" : "0") +
                                  "\nzoom_fov_deg=" + number(settings.zoom_fov_deg) + "\n");
}

CameraAlignment load_camera_alignment(const std::filesystem::path& file) {
    CameraAlignment alignment;
    const auto pairs = read_pairs(file);
    read_number(pairs, "fov_deg", 20.0, 120.0, alignment.fov_deg);
    read_number(pairs, "right_offset_deg", -15.0, 15.0, alignment.right_offset_deg);
    read_number(pairs, "up_offset_deg", -15.0, 15.0, alignment.up_offset_deg);
    read_bool(pairs, "measured", alignment.measured);
    return alignment;
}

bool save_camera_alignment(const std::filesystem::path& file, const CameraAlignment& alignment) {
    return write_atomically(file, "# ZeroSky camera alignment: lens FOV and optical-axis offsets\n"
                                  "fov_deg=" + number(alignment.fov_deg) +
                                  "\nright_offset_deg=" + number(alignment.right_offset_deg) +
                                  "\nup_offset_deg=" + number(alignment.up_offset_deg) +
                                  "\nmeasured=" + (alignment.measured ? "1" : "0") + "\n");
}

} // namespace zerosky
