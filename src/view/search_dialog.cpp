/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "search_dialog.h"

#include "asset_manager.h"
#include "linux_input.h"
#include "theme.h"
#include "ui_const.h"

#include <utility>

namespace zerosky {
namespace {

constexpr size_t kVisibleRows = 5;
constexpr size_t kMaxQuery = 64;

} // namespace

SearchDialog::SearchDialog(app::AssetManager& assets, bool dark_mode,
                           std::vector<SearchItem> items,
                           std::function<void(const std::string&)> on_pick)
    : items_(std::move(items)), on_pick_(std::move(on_pick)), dark_(dark_mode) {
    const auto pal = view::palette(dark_);
    auto* heading_font = assets.load_font("inter-semibold.ttf", 14);
    auto* body_font = assets.load_font("inter-regular.ttf", 12);
    const lv_font_t* fallback = &lv_font_montserrat_12;

    root_ = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(root_);
    lv_obj_set_size(root_, view::kScreenWidth, view::kScreenHeight);
    lv_obj_set_style_bg_color(root_, pal.background, 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(root_, 5, 0);
    lv_obj_set_style_pad_row(root_, 2, 0);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);

    title_ = lv_label_create(root_);
    lv_obj_set_width(title_, LV_PCT(100));
    lv_obj_set_style_text_font(title_, heading_font ? heading_font : fallback, 0);
    lv_obj_set_style_text_color(title_, pal.primary, 0);

    input_ = lv_label_create(root_);
    lv_obj_set_width(input_, LV_PCT(100));
    lv_label_set_long_mode(input_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(input_, heading_font ? heading_font : fallback, 0);
    lv_obj_set_style_text_color(input_, pal.text, 0);
    lv_obj_set_style_border_side(input_, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(input_, 1, 0);
    lv_obj_set_style_border_color(input_, pal.border, 0);

    for (size_t i = 0; i < kVisibleRows; ++i) {
        auto* row = lv_label_create(root_);
        lv_obj_set_size(row, LV_PCT(100), 19);
        lv_label_set_long_mode(row, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(row, body_font ? body_font : fallback, 0);
        lv_obj_set_style_pad_hor(row, 3, 0);
        lv_obj_set_style_radius(row, 3, 0);
        rows_.push_back(row);
    }

    auto* hint = lv_label_create(root_);
    lv_obj_set_width(hint, LV_PCT(100));
    lv_label_set_long_mode(hint, LV_LABEL_LONG_DOT);
    lv_label_set_text(hint, "Type name  Up/Down pick  Enter OK  Esc cancel");
    lv_obj_set_style_text_font(hint, body_font ? body_font : fallback, 0);
    lv_obj_set_style_text_color(hint, pal.text_disabled, 0);

    search();
    platform::set_key_capture(key_cb, this, /*text=*/true);
}

SearchDialog::~SearchDialog() {
    close();
}

void SearchDialog::key_cb(uint32_t key, void* ctx) {
    if (auto* self = static_cast<SearchDialog*>(ctx)) self->on_key(key);
}

void SearchDialog::on_key(uint32_t key) {
    if (key == LV_KEY_ESC) {
        close();
        return;
    }
    if (key == LV_KEY_ENTER) {
        if (selected_ < matches_.size()) {
            const std::string name = items_[matches_[selected_]].name;
            const auto on_pick = on_pick_;
            close();
            if (on_pick) on_pick(name);
        }
        return;
    }
    if (key == LV_KEY_UP) {
        if (selected_ > 0) --selected_;
        render();
        return;
    }
    if (key == LV_KEY_DOWN) {
        if (selected_ + 1 < matches_.size()) ++selected_;
        render();
        return;
    }
    if (key == LV_KEY_BACKSPACE) {
        if (query_.empty()) return;
        query_.pop_back();
    } else if (key >= 0x20 && key < 0x7f && query_.size() < kMaxQuery) {
        query_.push_back(static_cast<char>(key));
    } else {
        return;
    }
    selected_ = 0;
    first_visible_ = 0;
    search();
}

void SearchDialog::search() {
    matches_ = filter_search_items(items_, query_);
    render();
}

void SearchDialog::render() {
    if (!root_) return;
    const auto pal = view::palette(dark_);
    lv_label_set_text_fmt(title_, "Find (%u)", static_cast<unsigned>(matches_.size()));
    lv_label_set_text_fmt(input_, "> %s_", query_.c_str());

    if (selected_ >= first_visible_ + rows_.size())
        first_visible_ = selected_ - rows_.size() + 1;
    if (selected_ < first_visible_) first_visible_ = selected_;

    for (size_t i = 0; i < rows_.size(); ++i) {
        auto* row = rows_[i];
        const size_t match = first_visible_ + i;
        if (match >= matches_.size() && !(matches_.empty() && i == 0)) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(row, LV_OBJ_FLAG_HIDDEN);
        const bool empty = matches_.empty();
        const bool above = !empty && items_[matches_[match]].above_horizon;
        const bool highlighted = !empty && match == selected_;
        if (empty) {
            lv_label_set_text(row, "No matching objects");
        } else {
            const auto& item = items_[matches_[match]];
            const std::string line = item.name + (above ? "" : " (below)") +
                                     (item.detail.empty() ? "" : "  " + item.detail);
            lv_label_set_text(row, line.c_str());
        }
        lv_obj_set_style_bg_opa(row, highlighted ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(row, pal.primary, 0);
        lv_obj_set_style_text_color(row,
            highlighted ? lv_color_black() : (above ? pal.text : pal.text_disabled), 0);
    }
}

void SearchDialog::close() {
    if (!root_) return;
    platform::set_key_capture(nullptr, nullptr);
    lv_obj_delete(root_);
    root_ = nullptr;
    title_ = nullptr;
    input_ = nullptr;
    rows_.clear();
}

} // namespace zerosky
