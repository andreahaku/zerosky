/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "attitude_controller.h"
#include "nav_provider.h"
#include "projection.h"
#include "shell_viewmodel.h"
#include "sky_engine.h"

#include <string>

namespace zerosky {

// The app view-model: the observer, the camera and the user actions. The
// arrows move the camera until the attitude service drives it on the device.
class SkyViewModel : public toolkit::ShellViewModel, public toolkit::NavProvider {
public:
    SkyViewModel();

    int nav_page_count() const override { return 3; }
    const char* nav_page_name(int page) const override {
        return page == 0 ? "Sky" : page == 1 ? "View" : "Cam";
    }
    void nav_fill(int page, NavSlot out[5]) const override;
    void nav_activate(int page, int slot) override;
    toolkit::i18n::Text nav_hint(int page, int slot) const override;
    bool nav_has_hold(int page, int slot) const override { return page == 0 && slot == 4; }
    std::string nav_hold(int page, int slot) override;

    // TAB switches between the camera from the IMU and the manual camera.
    void on_tab() override { manual_ = !manual_; }

    void on_left() override { turn(-kStepDeg, 0.0); }
    void on_right() override { turn(kStepDeg, 0.0); }
    void on_up() override { turn(0.0, kStepDeg); }
    void on_down() override { turn(0.0, -kStepDeg); }

    const sky::Camera& camera() const { return camera_; }
    // Daylight hides the faint stars; off, the sky is drawn as at night.
    bool daylight() const { return daylight_; }
    // Camera background (phase 2). While it is on, the field of view is the
    // lens one, not the zoom.
    bool camera_on() const { return camera_on_; }
    static double lens_fov_deg();

    // Night: red on black. Day: white and blue on dark blue.
    bool night_theme() const { return night_theme_; }
    const sky::Observer& observer() const { return observer_; }
    bool location_set() const { return location_set_; }
    const std::string& location_label() const { return location_label_; }

    // The screen opens the location dialog when this returns true, once.
    bool take_location_request();
    void set_location(double latitude_deg, double longitude_deg, std::string label);

    // Search: key 8 asks the screen for the search dialog, a hold clears the
    // target. The screen lists the objects, since it holds the sky positions.
    bool take_search_request();
    void set_target(std::string name);
    const std::string& target() const { return target_; }

    // Starts the IMU (device only) with the WMM coefficients at `cof_file`.
    void start_attitude(const std::filesystem::path& cof_file);
    // Called every frame: follows the IMU unless in manual mode.
    void update_camera();
    bool manual() const { return manual_ || !attitude_live_; }
    // Tilt only: the IMU steers the altitude and the roll, the gyro turns the
    // azimuth from where the arrows set it. Used until the compass is calibrated.
    bool tilt_only() const { return !manual() && !compass_usable_; }
    std::string attitude_status() const {
#if !USE_DESKTOP
        if (cof_missing_) return "no WMM data";
#endif
        return attitude_.status();
    }
    // The location dialog opens by itself once, at the first start.
    bool take_first_location_prompt() {
        const bool prompt = !location_set_ && !first_prompt_done_;
        first_prompt_done_ = true;
        return prompt;
    }

    // The magnetic hardware configuration (none, cap, usb_gps), kept in
    // ~/.config/zerosky/hardware. ZEROSKY_HARDWARE overrides it.
    const std::string& hardware() const { return hardware_; }
    // The screen opens the About page when this returns true, once.
    bool take_about_request();

    // Bumped whenever the observer changes, so the screen recomputes the sky.
    int observer_version() const { return observer_version_; }

private:
    static constexpr double kStepDeg = 5.0;
    void turn(double d_azimuth, double d_altitude);
    void restart_attitude();
    void cycle_hardware();
    void zoom(double factor);

    sky::Camera camera_{180.0, 30.0, 0.0, 60.0};
    sky::Observer observer_;
    bool location_set_{false};
    std::string location_label_;
    bool location_request_{false};
    int observer_version_{0};
    bool search_request_{false};
    std::string target_;
    AttitudeController attitude_;
    std::filesystem::path cof_file_;
    bool manual_{false};
    bool daylight_{true};
    bool night_theme_{true};
    bool camera_on_{false};
    double zoom_fov_deg_{60.0}; // the zoom to restore when the camera goes off
    std::string hardware_{"none"};
    bool about_request_{false};
    bool cof_missing_{false};
    bool first_prompt_done_{false};
    bool attitude_live_{false};
    bool compass_usable_{false};
    double azimuth_offset_{0.0}; // tilt only: added to the gyro azimuth
};

} // namespace zerosky
