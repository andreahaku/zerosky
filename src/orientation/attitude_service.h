/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "calibration.h"
#include "magnetic_gate.h"
#include "mahony.h"
#include "sensor_source.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace zerosky::orientation {

class GeomagneticModel;

struct FrameMapping {
    // These map raw IIO sensor frames into the physical device body frame.
    Matrix3 accel_to_body{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    Matrix3 gyro_to_body{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    sensors::Vec3 pointing_body{0, 0, 1};
    // Direction of the top edge of the display, perpendicular to pointing_body.
    sensors::Vec3 screen_up_body{0, 1, 0};
    bool verified = false;
};

struct MagneticReference {
    // WMM vector in ENU and microtesla: (east, north, up). Its declination is
    // already encoded in east/north; callers must not add D to output azimuth.
    sensors::Vec3 field_enu_uT;
    bool valid = false;
};

enum class CompassStatus { Unreliable, Ok, Disturbed };

struct AttitudeSnapshot {
    Quaternion body_to_enu;
    sensors::Sample raw_sample;
    sensors::Vec3 last_raw_magnetic_uT;
    std::chrono::steady_clock::time_point last_raw_magnetic_at;
    double pointing_azimuth_deg = 0.0;
    double pointing_altitude_deg = 0.0;
    CompassStatus compass = CompassStatus::Unreliable;
    bool simulated = false;
    bool accelerometer_used = false;
    bool magnetometer_used = false;
    std::chrono::steady_clock::time_point sampled_at;
    std::string error;
};

struct CalibrationSample {
    std::chrono::steady_clock::time_point sampled_at;
    sensors::Vec3 raw_magnetic_uT;
};

class AttitudeService {
public:
    explicit AttitudeService(std::unique_ptr<sensors::Source> source);
    ~AttitudeService();
    AttitudeService(const AttitudeService&) = delete;
    AttitudeService& operator=(const AttitudeService&) = delete;

    bool start(std::string& error);
    void stop();
    bool running() const { return running_.load(); }

    // A failed load selects the requested configuration but leaves the compass
    // unreliable. It does not fall back to another hardware profile.
    bool select_configuration(const std::string& hardware,
                              const std::filesystem::path& calibration_file,
                              std::string& error);
    bool set_frame_mapping(FrameMapping mapping, std::string& error);
    void set_magnetic_reference(MagneticReference reference);
    // Computes the local WMM reference and leaves the compass unreliable when
    // the position, date, or coefficient model is invalid.
    bool set_geomagnetic_location(const GeomagneticModel& model,
                                  double latitude_deg, double longitude_deg,
                                  double height_wgs84_m, double decimal_year,
                                  std::string& error);
    AttitudeSnapshot snapshot() const;

    // Captures fresh, uncalibrated BMM150 samples for a later ellipsoid fit.
    // Collection is bounded to 6000 samples and never writes a profile itself.
    bool begin_calibration_capture(const std::string& hardware, std::string& error);
    std::vector<CalibrationSample> end_calibration_capture();

private:
    void run();
    void process(const sensors::Sample& sample, const std::string& source_error);

    std::unique_ptr<sensors::Source> source_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    mutable std::mutex mutex_;
    FrameMapping mapping_;
    MagneticReference reference_;
    Calibration calibration_;
    bool calibration_valid_ = false;
    std::string hardware_;
    Mahony filter_;
    MagneticGate magnetic_gate_;
    std::chrono::steady_clock::time_point previous_sample_at_{};
    std::chrono::steady_clock::time_point last_magnetic_at_{};
    std::chrono::steady_clock::time_point heading_good_since_{};
    bool heading_converged_ = false;
    AttitudeSnapshot latest_;
    bool capturing_ = false;
    std::vector<CalibrationSample> captured_;
};

} // namespace zerosky::orientation
