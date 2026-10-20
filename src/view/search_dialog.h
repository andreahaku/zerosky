/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "search_filter.h"
#include "lvgl.h"

#include <functional>
#include <string>
#include <vector>

namespace app {
class AssetManager;
}

namespace zerosky {

// Full-screen search modal. The owner retains it until is_open() becomes
// false; closing releases key capture and deletes its LVGL objects.
class SearchDialog {
public:
    SearchDialog(app::AssetManager& assets, bool dark_mode,
                 std::vector<SearchItem> items,
                 std::function<void(const std::string& name)> on_pick);
    ~SearchDialog();
    SearchDialog(const SearchDialog&) = delete;
    SearchDialog& operator=(const SearchDialog&) = delete;

    bool is_open() const { return root_ != nullptr; }

private:
    static void key_cb(uint32_t key, void* ctx);
    void on_key(uint32_t key);
    void search();
    void render();
    void close();

    std::vector<SearchItem> items_;
    std::function<void(const std::string&)> on_pick_;
    bool dark_;
    std::string query_;
    std::vector<size_t> matches_;
    size_t selected_{0};
    size_t first_visible_{0};

    lv_obj_t* root_{nullptr};
    lv_obj_t* title_{nullptr};
    lv_obj_t* input_{nullptr};
    std::vector<lv_obj_t*> rows_;
};

} // namespace zerosky
