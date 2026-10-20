/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "star_catalog.h"

#include <cctype>
#include <cstdlib>
#include <fstream>

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

// strtod over the whole field; false when the field is empty or has trailing text.
bool parse_double(const std::string& text, double& out) {
    if (text.empty()) return false;
    char* end = nullptr;
    out = std::strtod(text.c_str(), &end);
    return end == text.c_str() + text.size();
}

bool equal_ignore_case(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

StarCatalog StarCatalog::load(const std::string& path) {
    StarCatalog catalog;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto f = split_tabs(line);
        if (f.size() != 8) continue;
        Star star;
        double ra = 0.0, dec = 0.0, mag = 0.0, bv = 0.0, hip = 0.0;
        if (!parse_double(f[1], ra) || !parse_double(f[2], dec) || !parse_double(f[3], mag)) continue;
        if (ra < 0.0 || ra >= 24.0 || dec < -90.0 || dec > 90.0) continue;
        if (parse_double(f[0], hip)) star.hip = static_cast<int>(hip);
        star.ra_hours = ra;
        star.dec_deg = dec;
        star.magnitude = static_cast<float>(mag);
        if (parse_double(f[4], bv)) {
            star.bv = static_cast<float>(bv);
            star.has_bv = true;
        }
        star.bayer = f[5];
        star.constellation = f[6];
        star.name = f[7];
        catalog.stars_.push_back(std::move(star));
    }
    return catalog;
}

const Star* StarCatalog::find_by_name(const std::string& name) const {
    for (const auto& star : stars_) {
        if (!star.name.empty() && equal_ignore_case(star.name, name)) return &star;
    }
    return nullptr;
}

} // namespace zerosky::sky
