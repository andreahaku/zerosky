/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "beeper.h"
#include "calibration_flow.h"
#include "lvgl.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace app { class AssetManager; }

namespace zerosky {
class SkyViewModel;

// Full-screen capture. Esc cancels without writing; the demo never saves.
class CalibrationDialog {
public:
    CalibrationDialog(SkyViewModel& vm, app::AssetManager& assets, audio::Beeper& beeper,
                      bool dark_mode);
    ~CalibrationDialog();
    CalibrationDialog(const CalibrationDialog&) = delete;
    CalibrationDialog& operator=(const CalibrationDialog&) = delete;

    bool is_open() const { return root_ != nullptr; }
    void tick();

private:
    static void key_cb(uint32_t key, void* context);
    void on_key(uint32_t key);
    void advance_demo();
    void render();
    void draw_capture(const orientation::CaptureCoverage& coverage, int seconds_left);
    void draw_result(bool success);
    void close();

    SkyViewModel& vm_;
    bool started_{false};
    bool saved_{false};
    bool reload_failed_{false};
    bool save_failed_{false};
    bool dark_{false};
    bool demo_{false};
    bool demo_failure_{false};
    std::string error_;
    orientation::CalibrationStage seen_stage_{orientation::CalibrationStage::Capturing};
    std::chrono::steady_clock::time_point animation_started_{};
    std::unique_ptr<orientation::CalibrationFlow> demo_flow_;
    std::vector<orientation::CalibrationSample> demo_samples_;
    std::size_t demo_cursor_{0};
    audio::Beeper& beeper_;
    std::vector<uint16_t> pixels_;
    lv_obj_t* root_{nullptr};
    lv_obj_t* canvas_{nullptr};
    lv_obj_t* title_{nullptr};
    lv_obj_t* detail_{nullptr};
    lv_obj_t* hint_{nullptr};
};

} // namespace zerosky
