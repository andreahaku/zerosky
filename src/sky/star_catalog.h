/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <vector>

namespace zerosky::sky {

// One star of assets/sky/stars.tsv (tools/catalog/build_stars.py), J2000.
struct Star {
    int hip{0};              // Hipparcos number, 0 when the star has none
    double ra_hours{0.0};
    double dec_deg{0.0};
    float magnitude{0.0f};
    float bv{0.0f};          // B-V colour index
    bool has_bv{false};
    std::string bayer;       // "Alp", "Bet-1", a Flamsteed number, or empty
    std::string constellation; // IAU abbreviation, e.g. "UMi"
    std::string name;        // proper name, e.g. "Polaris", or empty
};

// The catalog, brightest first. Loading a missing or unreadable file gives an
// empty catalog. Bad lines are skipped.
class StarCatalog {
public:
    static StarCatalog load(const std::string& path);

    const std::vector<Star>& stars() const { return stars_; }
    bool empty() const { return stars_.empty(); }

    // The first star whose proper name matches `name`, ignoring case, or nullptr.
    const Star* find_by_name(const std::string& name) const;

private:
    std::vector<Star> stars_;
};

} // namespace zerosky::sky
