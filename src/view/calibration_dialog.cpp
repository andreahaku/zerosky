/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "calibration_dialog.h"

#include "asset_manager.h"
#include "i18n.h"
#include "linux_input.h"
#include "sky_text.h"
#include "sky_viewmodel.h"
#include "theme.h"
#include "ui_const.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace zerosky {
namespace {

constexpr int kW = view::kScreenWidth;
constexpr int kH = view::kScreenHeight;
constexpr double kPi = 3.14159265358979323846;
using toolkit::i18n::pick;

const char* missing_pose(orientation::MissingPoseHint hint) {
    using orientation::MissingPoseHint;
    switch (hint) {
    case MissingPoseHint::ScreenDown: return pick(text::kCalibrationScreenDown);
    case MissingPoseHint::ScreenUp: return pick(text::kCalibrationScreenUp);
    case MissingPoseHint::KeyboardDown: return pick(text::kCalibrationKeyboardDown);
    case MissingPoseHint::TopDown: return pick(text::kCalibrationTopDown);
    case MissingPoseHint::RightDown: return pick(text::kCalibrationRightDown);
    case MissingPoseHint::LeftDown: return pick(text::kCalibrationLeftDown);
    case MissingPoseHint::TurnAllSides: return pick(text::kCalibrationTurnAllSides);
    }
    return pick(text::kCalibrationTurnAllSides);
}

const char* reason(const std::string& error) {
    if (error == "Set a position before calibrating the compass") return pick(text::kCalibrationNoPosition);
    if (error == "Frame axes are not measured; use zerosky_probe") return pick(text::kCalibrationNoFrame);
    if (error == "Magnetometer axes are not measured; use zerosky_probe --save-frames")
        return pick(text::kCalibrationNoMagFrame);
    if (error == "BMM150 magnetometer is not available") return pick(text::kCalibrationNoMag);
    if (error == "WMM2025 model is not available" ||
        error == "Cannot calculate WMM2025 field for this position and date")
        return pick(text::kCalibrationNoWmm);
    if (error == "Calibration needs at least 200 independent samples") return pick(text::kCalibrationFew);
    if (error == "Calibration does not cover all eight octants") return pick(text::kCalibrationOctants);
    if (error == "Magnetometer ellipsoid fit is degenerate" ||
        error == "Magnetometer ellipsoid is not positive definite") return pick(text::kCalibrationDegenerate);
    if (error == "Magnetometer ellipsoid axis ratio is too large") return pick(text::kCalibrationShape);
    if (error == "Held-out magnetic residual exceeds 5 uT" ||
        error == "Invalid held-out magnetic residual") return pick(text::kCalibrationResidual);
    return pick(text::kCalibrationInvalid);
}

void attach_cjk(app::AssetManager& assets, lv_font_t* font) {
    if (!font || !toolkit::i18n::needs_cjk_font(toolkit::i18n::current())) return;
    static constexpr const char* japanese[] = {
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf"};
    static constexpr const char* chinese[] = {
        "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc"};
    const auto& paths = toolkit::i18n::current() == toolkit::i18n::Lang::Ja ? japanese : chinese;
    for (const char* path : paths)
        if (lv_font_t* fallback = assets.load_font(path, 14)) { font->fallback = fallback; return; }
}

void pixel(std::vector<uint16_t>& image, int x, int y, uint16_t color) {
    if (x >= 0 && x < kW && y >= 0 && y < kH)
        image[static_cast<std::size_t>(y) * kW + x] = color;
}

void disc(std::vector<uint16_t>& image, int cx, int cy, int radius, uint16_t color) {
    for (int dy = -radius; dy <= radius; ++dy)
        for (int dx = -radius; dx <= radius; ++dx)
            if (dx * dx + dy * dy <= radius * radius)
                pixel(image, cx + dx, cy + dy, color);
}

void line(std::vector<uint16_t>& image, int x0, int y0, int x1, int y1,
          int radius, uint16_t color) {
    const int steps = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
    for (int i = 0; i <= steps; ++i) {
        const double f = steps ? static_cast<double>(i) / steps : 0.0;
        disc(image, static_cast<int>(std::lround(x0 + (x1 - x0) * f)),
             static_cast<int>(std::lround(y0 + (y1 - y0) * f)), radius, color);
    }
}

orientation::CalibrationSample demo_sample(std::size_t cursor) {
    // Shuffled Fibonacci sphere with hard-iron offset and mild soft-iron shape.
    constexpr std::size_t count = 257;
    const std::size_t i = (cursor * 73) % count;
    const double z = 1.0 - 2.0 * (static_cast<double>(i) + 0.5) / count;
    const double angle = i * 2.399963229728653;
    const double xy = std::sqrt(1.0 - z * z);
    return {std::chrono::steady_clock::time_point{} + std::chrono::milliseconds(cursor * 35),
            {14.0 + 46.0 * xy * std::cos(angle), -8.0 + 38.0 * xy * std::sin(angle),
             22.0 + 49.0 * z}};
}

} // namespace

CalibrationDialog::CalibrationDialog(SkyViewModel& vm, app::AssetManager& assets,
                                     audio::Beeper& beeper, bool dark_mode)
    : vm_(vm), dark_(dark_mode), beeper_(beeper) {
    (void)dark_mode;
    auto* heading_asset = assets.load_font("inter-semibold.ttf", 14);
    auto* body_asset = assets.load_font("inter-regular.ttf", 14);
    attach_cjk(assets, heading_asset);
    attach_cjk(assets, body_asset);
    const lv_font_t* heading = heading_asset ? heading_asset : &lv_font_montserrat_12;
    const lv_font_t* body = body_asset ? body_asset : &lv_font_montserrat_12;

    root_ = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(root_);
    lv_obj_set_size(root_, kW, kH);
    lv_obj_set_style_bg_color(root_, lv_color_hex(0x080409), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    pixels_.assign(static_cast<std::size_t>(kW) * kH, 0);
    canvas_ = lv_canvas_create(root_);
    lv_canvas_set_buffer(canvas_, pixels_.data(), kW, kH, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(canvas_, 0, 0);
    const auto label = [this](const lv_font_t* font, lv_color_t color, int y, int height) {
        auto* obj = lv_label_create(root_);
        lv_obj_set_size(obj, kW - 16, height);
        lv_obj_set_pos(obj, 8, y);
        lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(obj, font, 0);
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(obj, color, 0);
        return obj;
    };
    const lv_color_t accent = lv_color_hex(0xff3434);
    title_ = label(heading, accent, 4, 18);
    detail_ = label(body, lv_color_hex(0xffe8df), 96, 28);
    hint_ = label(body, accent, 148, 20);

    animation_started_ = std::chrono::steady_clock::now();
    const char* demo = std::getenv("ZEROSKY_CALIBRATION_DEMO");
    demo_ = demo && demo[0] && std::string(demo) != "0";
    demo_failure_ = demo_ && std::string(demo) == "fail";
    if (demo_) {
        constexpr orientation::Matrix3 measured{{{-1, 0, 0}, {0, 1, 0}, {0, 0, -1}}};
        demo_flow_ = std::make_unique<orientation::CalibrationFlow>(
            "demo", 44.9, measured, orientation::CalibrationMetadata{},
            std::filesystem::path{}, animation_started_, 60.0);
        started_ = true;
    } else started_ = vm_.begin_calibration(error_);
    render();
    platform::set_key_capture(key_cb, this);
}

CalibrationDialog::~CalibrationDialog() { close(); }

void CalibrationDialog::key_cb(uint32_t key, void* context) {
    if (auto* self = static_cast<CalibrationDialog*>(context)) self->on_key(key);
}

void CalibrationDialog::on_key(uint32_t key) {
    if (key == LV_KEY_ESC) { close(); return; }
    if (key != LV_KEY_ENTER || !started_ || saved_ || save_failed_ || demo_) return;
    if (vm_.calibration_status().stage != orientation::CalibrationStage::Ready) return;
    if (vm_.save_calibration_capture(error_)) {
        saved_ = true;
        reload_failed_ = !error_.empty();
    } else save_failed_ = true;
    render();
}

void CalibrationDialog::advance_demo() {
    if (!demo_flow_ || demo_flow_->stage() != orientation::CalibrationStage::Capturing) return;
    const std::size_t limit = demo_failure_ ? orientation::kCalibrationMinIndependent - 1 : 257;
    for (int n = 0; n < 4 && demo_cursor_ < limit; ++n, ++demo_cursor_)
        demo_samples_.push_back(demo_sample(demo_cursor_));
    if (demo_flow_->needs_preview(demo_samples_.size())) demo_flow_->update(demo_samples_);
    if (demo_flow_->should_finish(std::chrono::steady_clock::now()) ||
        (demo_failure_ && demo_cursor_ == limit))
        demo_flow_->finish(demo_samples_);
}

void CalibrationDialog::tick() {
    if (!root_ || !started_ || saved_ || save_failed_) return;
    if (demo_) advance_demo();
    render();
}

void CalibrationDialog::draw_capture(const orientation::CaptureCoverage& coverage,
                                     int seconds_left) {
    const uint16_t accent = lv_color_to_u16(lv_color_hex(0xff3434));
    const uint16_t dim = lv_color_to_u16(lv_color_hex(0x7b2424));
    const uint16_t moving = lv_color_to_u16(lv_color_hex(0xffe883));
    const double seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - animation_started_).count();
    for (int i = 0; i <= 240; ++i) {
        const double t = 2.0 * kPi * i / 240;
        disc(pixels_, 160 + static_cast<int>(std::lround(98 * std::sin(t))),
             61 + static_cast<int>(std::lround(25 * std::sin(2 * t))), 4, accent);
    }
    const double phase = std::fmod(seconds, 2.0) * kPi;
    disc(pixels_, 160 + static_cast<int>(std::lround(98 * std::sin(phase))),
         61 + static_cast<int>(std::lround(25 * std::sin(2 * phase))), 6, moving);
    for (int octant = 0; octant < 8; ++octant) {
        const int x = 69 + octant * 26;
        const bool covered = coverage.octant_counts[static_cast<std::size_t>(octant)] >=
                             orientation::kCalibrationMinPerOctant;
        disc(pixels_, x, 123, covered ? 6 : 5, covered ? accent : dim);
    }
    for (int y = 135; y < 146; ++y)
        for (int x = 24; x < 296; ++x) pixel(pixels_, x, y, dim);
    const int right = 24 + 272 * static_cast<int>(std::min<std::size_t>(
        coverage.independent, orientation::kCalibrationMinIndependent)) /
        static_cast<int>(orientation::kCalibrationMinIndependent);
    for (int y = 135; y < 146; ++y)
        for (int x = 24; x < right; ++x) pixel(pixels_, x, y, accent);
    lv_label_set_text_fmt(hint_, pick(text::kCalibrationProgress),
                          static_cast<unsigned>(std::min<std::size_t>(
                              coverage.independent, orientation::kCalibrationMinIndependent)),
                          seconds_left);
}

void CalibrationDialog::draw_result(bool success) {
    const uint16_t accent = lv_color_to_u16(lv_color_hex(0xff3434));
    if (success) {
        line(pixels_, 119, 65, 147, 86, 5, accent);
        line(pixels_, 147, 86, 204, 30, 5, accent);
    } else {
        line(pixels_, 134, 40, 186, 92, 5, accent);
        line(pixels_, 186, 40, 134, 92, 5, accent);
    }
}

void CalibrationDialog::render() {
    if (!root_) return;
    std::fill(pixels_.begin(), pixels_.end(), lv_color_to_u16(lv_color_hex(0x080409)));
    if (!started_) {
        lv_label_set_text(title_, pick(text::kCalibrationFailedTitle));
        draw_result(false);
        lv_label_set_text(detail_, reason(error_));
        lv_label_set_text(hint_, pick(text::kCalibrationCancelHint));
    } else {
        AttitudeController::CalibrationStatus status;
        if (demo_) {
            status.stage = demo_flow_->stage();
            status.coverage = demo_flow_->coverage();
            status.missing_pose = demo_flow_->missing_pose();
            status.seconds_left = demo_flow_->seconds_left(std::chrono::steady_clock::now());
            status.rms_uT = demo_flow_->result().rms_uT;
            status.field_uT = demo_flow_->field_uT();
            status.error = demo_flow_->error();
        } else status = vm_.calibration_status();
        if (seen_stage_ == orientation::CalibrationStage::Capturing &&
            (status.stage == orientation::CalibrationStage::Ready ||
             status.stage == orientation::CalibrationStage::Failed))
            beeper_.play_notification(status.stage == orientation::CalibrationStage::Ready);
        seen_stage_ = status.stage;
        if (saved_ || save_failed_ || status.stage != orientation::CalibrationStage::Capturing) {
            const bool success = saved_ || (!save_failed_ &&
                status.stage == orientation::CalibrationStage::Ready);
            lv_label_set_text(title_, pick(success ? text::kCalibrationDoneTitle
                                                   : text::kCalibrationFailedTitle));
            draw_result(success);
            if (saved_) {
                lv_label_set_text(detail_, pick(reload_failed_ ? text::kCalibrationReloadFailed
                                                               : text::kCalibrationSaved));
                lv_label_set_text(hint_, "");
            } else if (save_failed_) {
                lv_label_set_text(detail_, pick(text::kCalibrationSaveFailed));
                lv_label_set_text(hint_, pick(text::kCalibrationCancelHint));
            } else if (success) {
                lv_label_set_text_fmt(detail_, pick(text::kCalibrationReady),
                                      status.rms_uT, status.field_uT);
                lv_label_set_text(hint_, pick(demo_ ? text::kCalibrationDemoHint
                                                  : text::kCalibrationSaveHint));
            } else {
                lv_label_set_text(detail_, reason(status.error));
                lv_label_set_text(hint_, pick(demo_ ? text::kCalibrationDemoHint
                                                  : text::kCalibrationCancelHint));
            }
        } else {
            lv_label_set_text(title_, pick(text::kCalibrationShortGuide));
            lv_label_set_text(detail_, missing_pose(status.missing_pose));
            draw_capture(status.coverage, status.seconds_left);
        }
    }
    lv_obj_invalidate(canvas_);
}

void CalibrationDialog::close() {
    if (!root_) return;
    if (demo_) demo_flow_->cancel();
    else vm_.cancel_calibration_capture();
    beeper_.stop();
    platform::set_key_capture(nullptr, nullptr);
    lv_obj_delete(root_);
    root_ = nullptr;
}

} // namespace zerosky
