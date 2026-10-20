/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <vector>

namespace zerosky::sky {

enum class DeepSkyKind { Galaxy, OpenCluster, GlobularCluster, Nebula, PlanetaryNebula, Other };

// One object of assets/sky/deep_sky.tsv (tools/catalog/build_deep_sky.py,
// OpenNGC), J2000. Unknown numbers are NaN.
struct DeepSkyObject {
    std::string id;       // "NGC1976", "IC0434", ...
    std::string messier;  // "M42", or empty
    std::string type;     // OpenNGC type code: G, OCl, GCl, PN, HII, ...
    DeepSkyKind kind{DeepSkyKind::Other};
    double ra_hours{0.0};
    double dec_deg{0.0};
    double magnitude{0.0};
    double major_arcmin{0.0};
    std::string constellation;
    std::string name;     // the first common name, e.g. "Orion Nebula", or empty

    // What the search and the labels show: "M42", else the NGC/IC id as
    // "NGC 1976" without leading zeros.
    std::string label() const;
};

class DeepSkyCatalog {
public:
    // A missing file gives an empty catalog; malformed lines are skipped.
    static DeepSkyCatalog load(const std::string& path);

    const std::vector<DeepSkyObject>& objects() const { return objects_; }
    bool empty() const { return objects_.empty(); }

    // By Messier number ("M31", "m 31"), catalog id ("NGC 224", "ngc224") or
    // common name, ignoring case and spaces; nullptr when nothing matches.
    const DeepSkyObject* find(const std::string& query) const;

private:
    std::vector<DeepSkyObject> objects_;
};

} // namespace zerosky::sky
