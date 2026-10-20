/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

// Loads the shipped catalog and checks a few well-known stars, plus the parser
// on a small file with bad lines.

#include "sky_engine.h"
#include "star_catalog.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace zerosky::sky;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void shipped_catalog() {
    const auto catalog = StarCatalog::load(STARS_TSV);
    require(catalog.stars().size() > 5000, "catalog too small");
    require(catalog.stars().front().name == "Sirius", "brightest star is not Sirius");
    for (size_t i = 1; i < catalog.stars().size(); ++i) {
        require(catalog.stars()[i - 1].magnitude <= catalog.stars()[i].magnitude,
                "catalog not sorted by magnitude");
    }
    const Star* polaris = catalog.find_by_name("polaris");
    require(polaris && polaris->constellation == "UMi" && polaris->bayer == "Alp", "Polaris missing");
    require(std::abs(polaris->dec_deg - 89.264) < 0.01, "Polaris declination off");

    // Polaris from the catalog, through the sky engine, sits at the latitude.
    const Observer gozo{36.05, 14.24, 50.0};
    const Horizontal h = star_horizontal(polaris->ra_hours, polaris->dec_deg, 1782000000.0, gozo);
    require(std::abs(h.altitude_deg - gozo.latitude_deg) < 0.8, "Polaris altitude off");
}

void parser_skips_bad_lines() {
    const auto path = std::filesystem::temp_directory_path() / "zerosky_star_catalog_test.tsv";
    {
        std::ofstream out(path);
        out << "# header\n"
            << "1\t1.5\t10.0\t2.00\t0.5\tAlp\tAnd\tGood\n"
            << "2\t25.0\t10.0\t2.00\t\t\tAnd\t\n"      // ra out of range
            << "3\tx\t10.0\t2.00\t\t\tAnd\t\n"         // ra not a number
            << "4\t1.0\t10.0\n"                        // too few fields
            << "\t2.0\t-5.0\t3.10\t\t\tOri\t\n";       // no hip, no colour: kept
    }
    const auto catalog = StarCatalog::load(path.string());
    std::filesystem::remove(path);
    require(catalog.stars().size() == 2, "expected 2 good lines");
    require(catalog.stars()[0].has_bv && catalog.stars()[0].name == "Good", "first line wrong");
    require(!catalog.stars()[1].has_bv && catalog.stars()[1].hip == 0, "second line wrong");
    require(StarCatalog::load("/nonexistent/stars.tsv").empty(), "missing file not empty");
}

} // namespace

int main() {
    try {
        shipped_catalog();
        parser_skips_bad_lines();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("star_catalog_test: ok");
    return 0;
}
