/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "constellations.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <utility>

namespace zerosky::sky {
namespace {

std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> fields;
    size_t start = 0;
    while (true) {
        const size_t tab = line.find('\t', start);
        fields.push_back(line.substr(start, tab - start));
        if (tab == std::string::npos) break;
        start = tab + 1;
    }
    return fields;
}

bool parse_int(const std::string& text, int& value) {
    if (text.empty()) return false;
    char* end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (end != text.c_str() + text.size() || parsed < 0 ||
        parsed > std::numeric_limits<int>::max()) return false;
    value = static_cast<int>(parsed);
    return true;
}

bool parse_double(const std::string& text, double& value) {
    if (text.empty()) return false;
    char* end = nullptr;
    value = std::strtod(text.c_str(), &end);
    return end == text.c_str() + text.size() && std::isfinite(value);
}

bool parse_vertex(const std::vector<std::string>& fields, size_t at,
                  ConstellationVertex& vertex) {
    return parse_int(fields[at], vertex.hip) &&
           parse_double(fields[at + 1], vertex.ra_hours) &&
           parse_double(fields[at + 2], vertex.dec_deg) &&
           vertex.ra_hours >= 0.0 && vertex.ra_hours < 24.0 &&
           vertex.dec_deg >= -90.0 && vertex.dec_deg <= 90.0;
}

bool equal_ignore_case(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) return false;
    }
    return true;
}

} // namespace

ConstellationLines ConstellationLines::load(const std::string& path) {
    ConstellationLines lines;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto fields = split_tabs(line);
        if (fields.size() != 8 || fields[0].size() != 3) continue;
        ConstellationSegment segment;
        segment.abbreviation = fields[0];
        if (!parse_int(fields[1], segment.rank) || segment.rank < 1 || segment.rank > 3 ||
            !parse_vertex(fields, 2, segment.a) || !parse_vertex(fields, 5, segment.b)) continue;
        lines.segments_.push_back(std::move(segment));
    }
    return lines;
}

std::vector<const ConstellationSegment*> ConstellationLines::find_by_abbreviation(
    const std::string& abbreviation) const {
    std::vector<const ConstellationSegment*> matches;
    for (const auto& segment : segments_) {
        if (equal_ignore_case(segment.abbreviation, abbreviation)) matches.push_back(&segment);
    }
    return matches;
}

} // namespace zerosky::sky
