/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "projection.h"
#include "sky_engine.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace zerosky::orientation {
class AttitudeService;
class GeomagneticModel;
struct FrameMapping;
} // namespace zerosky::orientation

namespace zerosky {

// Drives the camera from the IMU on the device. It starts only when the axes
// were measured (frame-mapping.json, see docs/probe.md); otherwise it reports
// why, and the app stays in manual mode.
class AttitudeController {
public:
    AttitudeController();
    ~AttitudeController();
    AttitudeController(const AttitudeController&) = delete;
    AttitudeController& operator=(const AttitudeController&) = delete;

    // Starts the sensors for `hardware` (none, cap, usb_gps, ...). Safe to call
    // again when the observer changes: only the magnetic reference is updated.
    void start(const sky::Observer& observer, const std::string& hardware,
               const std::filesystem::path& cof_file);

    // The camera from the latest attitude, or nothing while unavailable.
    // Without a usable compass its azimuth only follows the gyro, from an
    // arbitrary start: see compass_usable().
    std::optional<sky::Camera> camera(const sky::Camera& previous) const;
    bool compass_usable() const;

    // Switches the magnetic profile; the compass stays unreliable until it loads.
    void select_hardware(const std::string& hardware);

    // A short status for the screen: "", "axes not measured", "compass: calibrate"...
    std::string status() const;

private:
    std::unique_ptr<orientation::AttitudeService> service_;
    std::unique_ptr<orientation::GeomagneticModel> model_;
    std::unique_ptr<orientation::FrameMapping> mapping_;
    std::string unavailable_;
};

} // namespace zerosky
