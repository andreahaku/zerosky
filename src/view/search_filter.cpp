/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "search_filter.h"

#include <algorithm>
#include <cctype>

namespace zerosky {
namespace {

char ascii_lower(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

bool word_char(char c) {
    const auto uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) != 0;
}

std::string normalized(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const char c : text) result.push_back(ascii_lower(c));
    return result;
}

std::string trim_lower(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
        text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
        text.remove_suffix(1);
    return normalized(text);
}

bool word_prefix(const std::string& name, const std::string& query) {
    if (query.empty()) return true;
    for (size_t pos = name.find(query); pos != std::string::npos;
         pos = name.find(query, pos + 1)) {
        if (pos == 0 || !word_char(name[pos - 1])) return true;
    }
    return false;
}

} // namespace

std::vector<size_t> filter_search_items(const std::vector<SearchItem>& items,
                                        std::string_view query) {
    const auto needle = trim_lower(query);
    struct Match {
        size_t index;
        bool prefix;
    };
    std::vector<Match> matches;
    matches.reserve(items.size());
    for (size_t i = 0; i < items.size(); ++i) {
        const auto name = normalized(items[i].name);
        if (name.find(needle) == std::string::npos) continue;
        matches.push_back({i, word_prefix(name, needle)});
    }
    std::stable_sort(matches.begin(), matches.end(), [&items](const Match& a, const Match& b) {
        if (a.prefix != b.prefix) return a.prefix;
        return items[a.index].above_horizon && !items[b.index].above_horizon;
    });
    std::vector<size_t> ordered;
    ordered.reserve(matches.size());
    for (const auto match : matches) ordered.push_back(match.index);
    return ordered;
}

} // namespace zerosky
