/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "constellation_names.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace zerosky::sky {
namespace {

std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> fields;
    size_t start = 0;
    while (true) {
        const size_t tab = line.find('\t', start);
        fields.push_back(line.substr(start, tab - start));
        if (tab == std::string::npos) return fields;
        start = tab + 1;
    }
}

bool number(const std::string& text, double& out) {
    if (text.empty()) return false;
    char* end = nullptr;
    out = std::strtod(text.c_str(), &end);
    return end == text.c_str() + text.size() && std::isfinite(out);
}

} // namespace

ConstellationNames ConstellationNames::load(const std::string& path) {
    ConstellationNames catalog;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto fields = split_tabs(line);
        if (fields.size() != 5 || fields[0].size() != 3 || fields[1].empty()) continue;
        ConstellationName name;
        name.abbreviation = fields[0];
        name.latin_name = fields[1];
        double rank = 0.0;
        if (!number(fields[2], rank) || rank < 1.0 || rank > 3.0 ||
            std::floor(rank) != rank || !number(fields[3], name.ra_hours) ||
            !number(fields[4], name.dec_deg) || name.ra_hours < 0.0 ||
            name.ra_hours >= 24.0 || name.dec_deg < -90.0 || name.dec_deg > 90.0) continue;
        name.rank = static_cast<int>(rank);
        catalog.names_.push_back(std::move(name));
    }
    return catalog;
}

} // namespace zerosky::sky
