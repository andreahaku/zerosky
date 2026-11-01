/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "constellation_names.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>

using namespace zerosky::sky;

namespace {
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
} // namespace

int main() {
    try {
        const auto catalog = ConstellationNames::load(NAMES_TSV);
        std::set<std::string> abbreviations;
        int serpens = 0;
        for (const auto& name : catalog.names()) {
            abbreviations.insert(name.abbreviation);
            if (name.abbreviation == "Ser") {
                require(name.latin_name == "Serpens", "Serpens Latin name changed");
                ++serpens;
            }
        }
        require(catalog.names().size() == 89 && abbreviations.size() == 88 && serpens == 2,
                "shipped labels do not cover the 88 constellations");
        const auto file = std::filesystem::temp_directory_path() / "zerosky_constellation_names_test.tsv";
        {
            std::ofstream out(file);
            out << "# header\nOri\tOrion\t1\t5.0\t0.0\n"
                << "Ori\t\t1\t5.0\t0.0\n"
                << "Ori\tOrion\t1\tNaN\t0.0\n"
                << "Ori\tOrion\t4\t5.0\t0.0\n"
                << "Ori\tOrion\t1\t24.0\t0.0\n";
        }
        require(ConstellationNames::load(file.string()).names().size() == 1,
                "malformed label rows were accepted");
        std::filesystem::remove(file);
        require(ConstellationNames::load("/nonexistent/constellation_names.tsv").empty(),
                "missing names file was not empty");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::puts("constellation_names_test: ok");
    return 0;
}
