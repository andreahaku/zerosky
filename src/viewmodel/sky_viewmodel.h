/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "attitude_controller.h"
#include "settings_store.h"
#include "nav_provider.h"
#include "projection.h"
#include "shell_viewmodel.h"
#include "sky_engine.h"

#include <optional>
#include <string>

namespace zerosky {

// The app view-model: the observer, the camera and the user actions. The
// arrows move the camera until the attitude service drives it on the device.
class SkyViewModel : public toolkit::ShellViewModel, public toolkit::NavProvider {
public:
    SkyViewModel();
    ~SkyViewModel() override;

    int nav_page_count() const override { return 4; }
    const char* nav_page_name(int page) const override;
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

    // The camera the screen draws with: the IMU or manual camera, plus the
    // optical-axis offsets of the camera alignment while the image is on.
    sky::Camera camera() const;
    // Daylight hides the faint stars; off, the sky is drawn as at night.
    bool daylight() const { return daylight_; }
    ConstellationDisplay constellation_display() const { return constellations_; }
    bool deep_sky_visible() const { return deep_sky_visible_; }
    // Camera background (phase 2). While it is on, the field of view is the
    // lens one, not the zoom.
    bool camera_on() const { return camera_on_; }
    double lens_fov_deg() const;
    // Align mode (Cam page): the arrows move the optical axis and keys 7/8 the
    // lens FOV, so drawn stars sit on the photographed ones. Saved on leaving.
    bool aligning() const { return aligning_; }
    const CameraAlignment& alignment() const { return alignment_; }

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
    bool take_calibration_request();
    bool begin_calibration(std::string& error);
    AttitudeController::CalibrationStatus calibration_status() {
        return attitude_.calibration_status();
    }
    bool save_calibration_capture(std::string& error) {
        return attitude_.save_calibration_capture(error);
    }
    void cancel_calibration_capture() { attitude_.cancel_calibration_capture(); }

    // The language picked on the Setup page (nullopt = follow the system),
    // saved for the whole suite. It applies at the next start: the counter
    // tells the screen to say so.
    const std::optional<toolkit::i18n::Lang>& language_choice() const { return language_choice_; }
    int language_version() const { return language_version_; }
    // False when the last language change could not be written to disk.
    bool language_saved() const { return language_saved_; }

    // Bumped whenever the observer changes, so the screen recomputes the sky.
    int observer_version() const { return observer_version_; }

private:
    static constexpr double kStepDeg = 5.0;
    void turn(double d_azimuth, double d_altitude);
    void restart_attitude();
    void save_settings() const;
    void set_camera_on(bool on);
    void set_aligning(bool on);
    void cycle_hardware();
    void cycle_language();
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
    ConstellationDisplay constellations_{ConstellationDisplay::Names};
    bool deep_sky_visible_{true};
    bool night_theme_{true};
    bool camera_on_{false};
    bool aligning_{false};
    CameraAlignment alignment_at_start_; // to tell whether Done changed anything
    bool zoom_dirty_{false};             // the zoom is saved on exit, not on every key
    CameraAlignment alignment_;
    double zoom_fov_deg_{60.0}; // the zoom to restore when the camera goes off
    std::string hardware_{"none"};
    bool about_request_{false};
    bool calibration_request_{false};
    std::optional<toolkit::i18n::Lang> language_choice_;
    int language_version_{0};
    bool language_saved_{true};
    bool cof_missing_{false};
    bool first_prompt_done_{false};
    bool attitude_live_{false};
    bool compass_usable_{false};
    double azimuth_offset_{0.0}; // tilt only: added to the gyro azimuth
};

} // namespace zerosky
