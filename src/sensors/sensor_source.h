/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <string>

namespace zerosky::sensors {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

// Values are in the IIO sensor frames, before the sensor-to-body rotations.
struct Sample {
    std::chrono::steady_clock::time_point sampled_at;
    Vec3 acceleration_mps2;
    Vec3 angular_velocity_radps;
    Vec3 magnetic_uT;
    bool magnetic_present = false;
    bool magnetic_fresh = false;
};

struct SensorInfo {
    std::string accelerometer_gyro_name;
    std::string magnetometer_name;
    bool magnetic_present = false;
    // Exposed IIO metadata, not necessarily applied to raw readings.
    std::string magnetometer_mount_matrix;
};

class Source {
public:
    virtual ~Source() = default;
    virtual bool read(Sample& sample, std::string& error) = 0;
    virtual bool simulated() const = 0;
};

// Synchronous, non-blocking-by-design sysfs polling. Call from a worker at a
// bounded rate; sysfs file reads themselves can still block briefly.
class IioSource final : public Source {
public:
    explicit IioSource(std::filesystem::path iio_root = "/sys/bus/iio/devices");

    // Finds devices by their IIO "name", not by an unstable device number.
    // The BMI270 is required; a missing BMM150 leaves six-axis data available.
    bool discover(std::string& error);
    const SensorInfo& info() const { return info_; }
    bool read(Sample& sample, std::string& error) override;
    bool simulated() const override { return false; }

private:
    std::filesystem::path root_;
    std::filesystem::path imu_path_;
    std::filesystem::path mag_path_;
    SensorInfo info_;
    double accel_scale_ = 0.0;
    double gyro_scale_ = 0.0;
    double mag_scale_ = 0.0;
    Vec3 accel_offset_;
    Vec3 gyro_offset_;
    Vec3 mag_offset_;
    std::chrono::steady_clock::time_point last_magnetic_read_{};
};

// Explicitly simulated motion for the desktop UI. Never reports real hardware.
class SimulatedSource final : public Source {
public:
    SimulatedSource();
    bool read(Sample& sample, std::string& error) override;
    bool simulated() const override { return true; }

private:
    std::chrono::steady_clock::time_point started_at_;
    std::chrono::steady_clock::time_point last_magnetic_read_{};
};

} // namespace zerosky::sensors
