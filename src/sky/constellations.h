/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <vector>

namespace zerosky::sky {

// One endpoint of a western figure segment. HIP 0 means no trustworthy match
// in the shipped mag-6 HYG catalog; use the J2000 coordinates instead.
struct ConstellationVertex {
    int hip{0};
    double ra_hours{0.0};
    double dec_deg{0.0};
};

struct ConstellationSegment {
    std::string abbreviation; // IAU three-letter abbreviation
    int rank{0};               // d3-celestial line rank, 1 (prominent) to 3
    ConstellationVertex a;
    ConstellationVertex b;
};

// A missing file produces an empty set; malformed rows are skipped.
class ConstellationLines {
public:
    static ConstellationLines load(const std::string& path);

    const std::vector<ConstellationSegment>& segments() const { return segments_; }
    bool empty() const { return segments_.empty(); }
    std::vector<const ConstellationSegment*> find_by_abbreviation(
        const std::string& abbreviation) const;

private:
    std::vector<ConstellationSegment> segments_;
};

} // namespace zerosky::sky
