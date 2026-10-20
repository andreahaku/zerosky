/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace zerosky {

struct SearchItem {
    std::string name;
    std::string detail;
    bool above_horizon{false};
};

// Indices into `items`, ordered by a word starting with `query`, then by a
// substring anywhere in the name. Within each group, visible objects come
// first and the input order is preserved. Empty query includes every item.
std::vector<size_t> filter_search_items(const std::vector<SearchItem>& items,
                                        std::string_view query);

} // namespace zerosky
