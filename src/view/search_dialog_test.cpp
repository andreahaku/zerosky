/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

// Host-only test of the pure ordering used by SearchDialog (no LVGL/audio).
#include "search_filter.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

using zerosky::SearchItem;
using zerosky::filter_search_items;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void search_order() {
    const std::vector<SearchItem> items = {
        {"AJupiter", "hidden substring", true},
        {"Jupiter", "planet", false},
        {"Jupiter B", "star", true},
        {"Jupiter A", "star", true},
        {"Blue Jupiter", "star", false},
        {"No match", "star", true},
    };
    require(filter_search_items(items, "jUp") == std::vector<size_t>({2, 3, 1, 4, 0}),
            "word prefix, horizon and stable order");
    require(filter_search_items(items, "  JUP  ") == std::vector<size_t>({2, 3, 1, 4, 0}),
            "case and whitespace");
    require(filter_search_items(items, "iter") == std::vector<size_t>({0, 2, 3, 1, 4}),
            "substring order");
    require(filter_search_items(items, "xyz").empty(), "unknown term");
    require(filter_search_items(items, "") == std::vector<size_t>({0, 2, 3, 5, 1, 4}),
            "empty query shows all, visible first");
}

} // namespace

int main() {
    try {
        search_order();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("search_dialog_test: ok");
    return 0;
}
