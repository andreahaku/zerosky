/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <vector>

namespace zerosky::sky {

struct ConstellationName {
    std::string abbreviation;
    std::string latin_name;
    int rank{0};
    double ra_hours{0.0}; // J2000 label point
    double dec_deg{0.0};
};

class ConstellationNames {
public:
    static ConstellationNames load(const std::string& path);
    const std::vector<ConstellationName>& names() const { return names_; }
    bool empty() const { return names_.empty(); }

private:
    std::vector<ConstellationName> names_;
};

} // namespace zerosky::sky
