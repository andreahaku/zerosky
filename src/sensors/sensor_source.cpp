/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "sensor_source.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace zerosky::sensors {
namespace {

constexpr auto kMagneticPeriod = std::chrono::milliseconds(50);

bool read_text(const std::filesystem::path& path, std::string& value) {
    std::ifstream stream(path);
    if (!stream) return false;
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    if (!stream.good() && !stream.eof()) return false;
    value = buffer.str();
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) value.pop_back();
    return !value.empty();
}

bool read_number(const std::filesystem::path& path, double& value) {
    std::ifstream stream(path);
    double parsed = 0.0;
    if (!(stream >> parsed) || !std::isfinite(parsed)) return false;
    stream >> std::ws;
    if (!stream.eof()) return false;
    value = parsed;
    return true;
}

bool read_axis(const std::filesystem::path& device, const std::string& prefix,
               double scale, const Vec3& offset, Vec3& out, std::string& error) {
    double raw[3]{};
    const char axes[3] = {'x', 'y', 'z'};
    for (int i = 0; i < 3; ++i) {
        const auto name = prefix + axes[i] + "_raw";
        if (!read_number(device / name, raw[i])) {
            error = "Cannot read IIO channel: " + (device / name).string();
            return false;
        }
    }
    out = {(raw[0] + offset.x) * scale,
           (raw[1] + offset.y) * scale,
           (raw[2] + offset.z) * scale};
    return true;
}

bool read_offsets(const std::filesystem::path& device, const std::string& prefix,
                  Vec3& out, std::string& error) {
    double global = 0.0;
    std::error_code ec;
    const auto global_path = device / (prefix + "offset");
    if (std::filesystem::exists(global_path, ec) &&
        !read_number(global_path, global)) {
        error = "Invalid IIO offset: " + global_path.string();
        return false;
    }
    if (ec) {
        error = "Cannot inspect IIO offset: " + global_path.string();
        return false;
    }
    double axis[3] = {global, global, global};
    const char names[3] = {'x', 'y', 'z'};
    for (int i = 0; i < 3; ++i) {
        const auto path = device / (prefix + names[i] + "_offset");
        if (std::filesystem::exists(path, ec) && !read_number(path, axis[i])) {
            error = "Invalid IIO offset: " + path.string();
            return false;
        }
        if (ec) {
            error = "Cannot inspect IIO offset: " + path.string();
            return false;
        }
    }
    out = {axis[0], axis[1], axis[2]};
    return true;
}

bool valid_scale(const std::filesystem::path& path, double& scale, std::string& error) {
    if (!read_number(path, scale) || scale <= 0.0) {
        error = "Invalid or missing IIO scale: " + path.string();
        return false;
    }
    return true;
}

} // namespace

IioSource::IioSource(std::filesystem::path iio_root) : root_(std::move(iio_root)) {}

bool IioSource::discover(std::string& error) {
    error.clear();
    imu_path_.clear();
    mag_path_.clear();
    info_ = {};
    last_magnetic_read_ = {};

    std::error_code ec;
    std::filesystem::directory_iterator entries(root_, ec);
    if (ec) {
        error = "Cannot scan IIO devices: " + root_.string() + ": " + ec.message();
        return false;
    }
    for (const auto& entry : entries) {
        if (!entry.is_directory(ec) || ec) {
            ec.clear();
            continue;
        }
        if (entry.path().filename().string().rfind("iio:device", 0) != 0) continue;
        std::string name;
        if (!read_text(entry.path() / "name", name)) continue;
        if (name.find("bmi270") != std::string::npos &&
            name.find("magn") == std::string::npos && imu_path_.empty()) {
            imu_path_ = entry.path();
            info_.accelerometer_gyro_name = name;
        } else if ((name.find("bmm150") != std::string::npos ||
                    name.find("bmc150_magn") != std::string::npos) && mag_path_.empty()) {
            mag_path_ = entry.path();
            info_.magnetometer_name = name;
        }
    }

    if (imu_path_.empty()) {
        error = "BMI270 IIO device not found under " + root_.string();
        return false;
    }
    if (!valid_scale(imu_path_ / "in_accel_scale", accel_scale_, error) ||
        !valid_scale(imu_path_ / "in_anglvel_scale", gyro_scale_, error) ||
        !read_offsets(imu_path_, "in_accel_", accel_offset_, error) ||
        !read_offsets(imu_path_, "in_anglvel_", gyro_offset_, error)) {
        imu_path_.clear();
        return false;
    }

    if (!mag_path_.empty()) {
        if (valid_scale(mag_path_ / "in_magn_scale", mag_scale_, error) &&
            read_offsets(mag_path_, "in_magn_", mag_offset_, error)) {
            info_.magnetic_present = true;
            read_text(mag_path_ / "in_mount_matrix", info_.magnetometer_mount_matrix);
        } else {
            // Invalid magnetometer metadata should not hide valid six-axis data.
            mag_path_.clear();
        }
    }
    return true;
}

bool IioSource::read(Sample& sample, std::string& error) {
    error.clear();
    if (imu_path_.empty()) {
        error = "IIO sensors have not been discovered";
        return false;
    }
    Sample next;
    const auto start = std::chrono::steady_clock::now();
    if (!read_axis(imu_path_, "in_accel_", accel_scale_, accel_offset_,
                   next.acceleration_mps2, error) ||
        !read_axis(imu_path_, "in_anglvel_", gyro_scale_, gyro_offset_,
                   next.angular_velocity_radps, error)) return false;

    next.magnetic_present = info_.magnetic_present;
    if (info_.magnetic_present &&
        (last_magnetic_read_ == std::chrono::steady_clock::time_point{} ||
         start - last_magnetic_read_ >= kMagneticPeriod)) {
        // IIO magnetometer scale is gauss per LSB; 1 gauss = 100 microtesla.
        if (read_axis(mag_path_, "in_magn_", mag_scale_ * 100.0, mag_offset_,
                      next.magnetic_uT, error)) {
            next.magnetic_fresh = true;
            last_magnetic_read_ = start;
        } else {
            // Keep six-axis readings available; the caller sees the diagnostic.
            next.magnetic_fresh = false;
        }
    }
    next.sampled_at = start + (std::chrono::steady_clock::now() - start) / 2;
    sample = next;
    return true;
}

SimulatedSource::SimulatedSource() : started_at_(std::chrono::steady_clock::now()) {}

bool SimulatedSource::read(Sample& sample, std::string& error) {
    error.clear();
    const auto now = std::chrono::steady_clock::now();
    const double seconds = std::chrono::duration<double>(now - started_at_).count();
    const double yaw = 0.15 * std::sin(0.5 * seconds);
    sample = {};
    sample.sampled_at = now;
    sample.acceleration_mps2 = {0.0, 0.0, 9.80665};
    sample.angular_velocity_radps = {0.0, 0.0, 0.075 * std::cos(0.5 * seconds)};
    sample.magnetic_present = true;
    if (last_magnetic_read_ == std::chrono::steady_clock::time_point{} ||
        now - last_magnetic_read_ >= kMagneticPeriod) {
        sample.magnetic_uT = {35.0 * std::sin(yaw), 35.0 * std::cos(yaw), -20.0};
        sample.magnetic_fresh = true;
        last_magnetic_read_ = now;
    }
    return true;
}

} // namespace zerosky::sensors
