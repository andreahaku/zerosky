/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "constellations.h"
#include "star_catalog.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>

using namespace zerosky::sky;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void shipped_lines() {
    const auto lines = ConstellationLines::load(LINES_TSV);
    const auto stars = StarCatalog::load(STARS_TSV);
    std::set<int> star_hips;
    for (const auto& star : stars.stars()) {
        if (star.hip > 0) star_hips.insert(star.hip);
    }
    std::set<std::string> abbreviations;
    size_t unmatched = 0;
    for (const auto& segment : lines.segments()) {
        abbreviations.insert(segment.abbreviation);
        for (const auto* vertex : {&segment.a, &segment.b}) {
            if (vertex->hip == 0) {
                ++unmatched;
            } else {
                require(star_hips.count(vertex->hip) == 1, "line refers to missing HIP star");
            }
        }
    }
    require(abbreviations.size() == 88, "expected all 88 IAU abbreviations");
    require(lines.segments().size() == 743, "segment count changed");
    require(unmatched > 0 && unmatched < 10, "unexpected unmatched vertices");
    require(!lines.find_by_abbreviation("ori").empty(), "Orion lookup failed");
    require(!lines.find_by_abbreviation("ser").empty(), "Serpens lookup failed");
    require(lines.find_by_abbreviation("xyz").empty(), "unknown abbreviation matched");
}

void parser_skips_bad_lines() {
    const auto path = std::filesystem::temp_directory_path() / "zerosky_constellations_test.tsv";
    {
        std::ofstream out(path);
        out << "# header\n"
            << "Ori\t1\t123\t5.5\t-1.0\t0\t5.6\t2.0\n"
            << "Ori\t1\t123\t5.5\t-1.0\n"
            << "Ori\t1\t123\tNaN\t-1.0\t0\t5.6\t2.0\n"
            << "Ori\t4\t123\t5.5\t-1.0\t0\t5.6\t2.0\n"
            << "Ori\t1\t123\t25.5\t-1.0\t0\t5.6\t2.0\n"
            << "Ori\t1\t-4\t5.5\t-1.0\t0\t5.6\t2.0\n"
            << "UMa\t2\t0\t11.0\t31.5\t55\t11.1\t33.1\n";
    }
    const auto lines = ConstellationLines::load(path.string());
    std::filesystem::remove(path);
    require(lines.segments().size() == 2, "expected two valid rows");
    require(lines.find_by_abbreviation("ORI").size() == 1, "case-insensitive lookup failed");
    require(lines.segments()[0].b.hip == 0, "coordinate fallback lost");
    require(ConstellationLines::load("/nonexistent/constellations.tsv").empty(),
            "missing file not empty");
}

} // namespace

int main() {
    try {
        shipped_lines();
        parser_skips_bad_lines();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("constellations_test: ok");
    return 0;
}
