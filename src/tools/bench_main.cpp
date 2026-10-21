/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

// zerosky_bench: times the sky work the view does, on the device CPU.
//   zerosky_bench [assets-root]   (default /usr/share/zerosky)
// Prints the cost of one sky refresh (every 10 s in the view) and of one
// frame of projection and ground fill (10 Hz).

#include "constellations.h"
#include "deep_sky.h"
#include "projection.h"
#include "sky_engine.h"
#include "star_catalog.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

using namespace zerosky::sky;
using Clock = std::chrono::steady_clock;

namespace {

double ms_since(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

} // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "/usr/share/zerosky";
    const auto stars = StarCatalog::load(root + "/sky/stars.tsv");
    const auto lines = ConstellationLines::load(root + "/sky/constellations.tsv");
    const auto deep = DeepSkyCatalog::load(root + "/sky/deep_sky.tsv");
    std::printf("loaded %zu stars, %zu segments, %zu deep-sky objects\n", stars.stars().size(),
                lines.segments().size(), deep.objects().size());
    if (stars.empty()) return 1;

    const Observer gozo{36.05, 14.24, 50.0};
    const double utc = 1782000000.0;
    constexpr int kRounds = 5;

    // One sky refresh: every star, both ends of every segment, every deep-sky
    // object, and the solar system.
    std::vector<Horizontal> out;
    auto start = Clock::now();
    for (int round = 0; round < kRounds; ++round) {
        out.clear();
        for (const auto& s : stars.stars()) out.push_back(star_horizontal(s.ra_hours, s.dec_deg, utc, gozo));
        for (const auto& seg : lines.segments()) {
            out.push_back(star_horizontal(seg.a.ra_hours, seg.a.dec_deg, utc, gozo));
            out.push_back(star_horizontal(seg.b.ra_hours, seg.b.dec_deg, utc, gozo));
        }
        for (const auto& o : deep.objects()) out.push_back(star_horizontal(o.ra_hours, o.dec_deg, utc, gozo));
        (void)solar_system(utc, gozo);
    }
    std::printf("sky refresh: %.1f ms (%zu conversions)\n", ms_since(start) / kRounds, out.size());

    // One frame: project every cached position and fill the ground per pixel.
    const Projection projection(Camera{180.0, 20.0, 0.0, 60.0}, 320, 170);
    int visible = 0;
    start = Clock::now();
    constexpr int kFrames = 50;
    for (int frame = 0; frame < kFrames; ++frame) {
        for (const auto& h : out) visible += projection.on_screen(projection.project(h.azimuth_deg, h.altitude_deg));
        for (int y = 0; y < 170; ++y) {
            for (int x = 0; x < 320; ++x) visible += projection.below_horizon(x + 0.5, y + 0.5);
        }
    }
    std::printf("frame (project + ground): %.2f ms (checksum %d)\n", ms_since(start) / kFrames, visible);
    return 0;
}
