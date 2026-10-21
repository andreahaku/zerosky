/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "base_screen.h"
#include "camera_source.h"
#include "beeper.h"
#include "constellations.h"
#include "deep_sky.h"
#include "location_dialog.h"
#include "search_dialog.h"
#include "sky_engine.h"
#include "sky_viewmodel.h"
#include "star_catalog.h"
#include "text_viewer.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace zerosky {

// The sky view: a full-screen RGB565 canvas in the red night theme, with the
// stars, the Sun, the Moon, the planets, the horizon and the cardinal points.
class SkyScreen : public screen::BaseScreen {
public:
    SkyScreen(SkyViewModel& vm, app::AssetManager& assets);
    ~SkyScreen() override;

protected:
    void build_content(lv_obj_t* content) override;
    bool show_title_bar() const override { return false; }
    bool overlay_nav_bar() const override { return true; }

private:
    static void tick_cb(lv_timer_t* timer);
    void tick();
    void update_sky(double utc);
    void render();
    void open_location_dialog();
    void open_about();
    void open_search();
    // The horizontal position of the object named `name`, if it is known.
    bool locate(const std::string& name, sky::Horizontal& out) const;
    void render_guidance(uint16_t* buf, const sky::Projection& projection);

    SkyViewModel& vm_;
    sky::StarCatalog catalog_;
    sky::ConstellationLines lines_;
    sky::DeepSkyCatalog deep_sky_;
    // Horizontal positions, recomputed every few seconds: the sky turns 0.25
    // degrees a minute, so the projection runs on cached positions every frame.
    std::vector<sky::Horizontal> star_positions_;
    std::vector<sky::BodyPosition> bodies_;
    std::vector<std::array<sky::Horizontal, 2>> line_positions_;
    std::vector<sky::Horizontal> deep_sky_positions_;
    double sky_utc_{0.0};
    double twilight_magnitude_{6.0};
    int sky_observer_version_{-1};

    std::vector<uint16_t> pixels_;
    lv_obj_t* canvas_{nullptr};
    lv_obj_t* status_{nullptr};
    lv_obj_t* target_label_{nullptr};
    int label_style_{-1}; // theme + photo the labels are painted for; -1 before the first frame
    audio::Beeper beeper_;
    std::array<lv_obj_t*, 8> cardinals_{};
    std::array<lv_obj_t*, 9> body_labels_{};
    lv_timer_t* timer_{nullptr};
    std::unique_ptr<toolkit::LocationDialog> location_dialog_;
    std::unique_ptr<view::widgets::TextViewer> about_;
    std::unique_ptr<SearchDialog> search_;
    std::unique_ptr<camera::CameraSource> camera_;
    camera::CameraFrame camera_frame_;
    std::string camera_logged_error_;
    void update_camera_source();
    // True when a fresh camera frame was copied into the canvas this frame.
    bool draw_camera_background();
};

} // namespace zerosky
