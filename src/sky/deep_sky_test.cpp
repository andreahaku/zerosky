/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

// Loads the shipped deep-sky catalog and checks well-known objects, plus the
// parser on a small file with bad lines.

#include "deep_sky.h"

#include <cmath>
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

void shipped_catalog() {
    const auto catalog = DeepSkyCatalog::load(DEEP_SKY_TSV);
    require(catalog.objects().size() > 500, "catalog too small");
    std::set<std::string> messier;
    for (const auto& obj : catalog.objects()) {
        if (!obj.messier.empty()) messier.insert(obj.messier);
    }
    require(messier.size() == 110, "not all 110 Messier objects");

    const auto* m31 = catalog.find("M31");
    require(m31 && m31->name == "Andromeda Galaxy" && m31->kind == DeepSkyKind::Galaxy, "M31");
    require(std::abs(m31->ra_hours - 0.7123) < 0.001 && std::abs(m31->dec_deg - 41.269) < 0.01,
            "M31 coordinates off");
    const auto* m42 = catalog.find("m 42");
    require(m42 && m42->kind == DeepSkyKind::Nebula && m42->label() == "M42", "M42");
    require(catalog.find("NGC 224") == m31 && catalog.find("ngc0224") == m31, "NGC id lookup");
    require(catalog.find("andromeda galaxy") == m31, "name lookup");
    require(catalog.find("M45") && catalog.find("M45")->kind == DeepSkyKind::OpenCluster, "M45");
    require(catalog.find("NGC 9999999") == nullptr, "unknown id found");
}

void parser_skips_bad_lines() {
    const auto path = std::filesystem::temp_directory_path() / "zerosky_deep_sky_test.tsv";
    {
        std::ofstream out(path);
        out << "# header\n"
            << "NGC0007\t\tG\t0.1\t-29.9\t\t2.0\t1.0\tScl\t\n"         // unknown mag: kept
            << "NGC0008\t\tG\t25.0\t10.0\t9.0\t\t\tPeg\t\n"           // ra out of range
            << "NGC0009\t\tG\tx\t10.0\t9.0\t\t\tPeg\t\n"              // ra not a number
            << "NGC0010\t\tG\t1.0\n"                                  // too few fields
            << "IC0434\t\tDrkN\t5.68\t-2.45\t\t\t\tOri\tHorsehead,B33\n";
    }
    const auto catalog = DeepSkyCatalog::load(path.string());
    std::filesystem::remove(path);
    require(catalog.objects().size() == 2, "expected 2 good lines");
    require(std::isnan(catalog.objects()[0].magnitude), "unknown magnitude is not NaN");
    require(catalog.objects()[1].name == "Horsehead" && catalog.objects()[1].label() == "IC 434",
            "name or label wrong");
    require(DeepSkyCatalog::load("/nonexistent/deep_sky.tsv").empty(), "missing file not empty");
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
    std::puts("deep_sky_test: ok");
    return 0;
}
