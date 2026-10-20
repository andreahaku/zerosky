/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "deep_sky.h"

#include <cctype>
#include <cmath>
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

// NaN for an empty field; false when the field is not a number.
bool parse_number(const std::string& text, double& out) {
    if (text.empty()) {
        out = NAN;
        return true;
    }
    char* end = nullptr;
    out = std::strtod(text.c_str(), &end);
    return end == text.c_str() + text.size();
}

DeepSkyKind kind_of(const std::string& type) {
    if (type == "G" || type == "GPair" || type == "GTrpl" || type == "GGroup") return DeepSkyKind::Galaxy;
    if (type == "OCl" || type == "*Ass") return DeepSkyKind::OpenCluster;
    if (type == "GCl") return DeepSkyKind::GlobularCluster;
    if (type == "PN") return DeepSkyKind::PlanetaryNebula;
    if (type == "Neb" || type == "HII" || type == "EmN" || type == "RfN" || type == "DrkN" ||
        type == "SNR" || type == "Cl+N") {
        return DeepSkyKind::Nebula;
    }
    return DeepSkyKind::Other;
}

// Lower case without spaces: "NGC 224" and "ngc224" compare equal.
std::string key(const std::string& text) {
    std::string out;
    for (const char c : text) {
        if (c != ' ') out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

// "NGC0224" -> "ngc224", the id key without leading zeros.
std::string id_key(const std::string& id) {
    size_t digits = 0;
    while (digits < id.size() && !std::isdigit(static_cast<unsigned char>(id[digits]))) ++digits;
    size_t first = digits;
    while (first + 1 < id.size() && id[first] == '0') ++first;
    return key(id.substr(0, digits) + id.substr(first));
}

} // namespace

std::string DeepSkyObject::label() const {
    if (!messier.empty()) return messier;
    size_t digits = 0;
    while (digits < id.size() && !std::isdigit(static_cast<unsigned char>(id[digits]))) ++digits;
    if (digits == 0 || digits == id.size()) return id;
    size_t first = digits;
    while (first + 1 < id.size() && id[first] == '0') ++first;
    return id.substr(0, digits) + " " + id.substr(first);
}

DeepSkyCatalog DeepSkyCatalog::load(const std::string& path) {
    DeepSkyCatalog catalog;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto f = split_tabs(line);
        if (f.size() != 10 || f[0].empty()) continue;
        DeepSkyObject obj;
        double size_minor = 0.0;
        if (!parse_number(f[3], obj.ra_hours) || !parse_number(f[4], obj.dec_deg) ||
            !parse_number(f[5], obj.magnitude) || !parse_number(f[6], obj.major_arcmin) ||
            !parse_number(f[7], size_minor)) {
            continue;
        }
        if (!(obj.ra_hours >= 0.0 && obj.ra_hours < 24.0 && obj.dec_deg >= -90.0 &&
              obj.dec_deg <= 90.0)) {
            continue;
        }
        obj.id = f[0];
        obj.messier = f[1];
        obj.type = f[2];
        obj.kind = kind_of(obj.type);
        obj.constellation = f[8];
        obj.name = f[9].substr(0, f[9].find(','));
        catalog.objects_.push_back(std::move(obj));
    }
    return catalog;
}

const DeepSkyObject* DeepSkyCatalog::find(const std::string& query) const {
    const std::string q = key(query);
    if (q.empty()) return nullptr;
    for (const auto& obj : objects_) {
        if ((!obj.messier.empty() && key(obj.messier) == q) || id_key(obj.id) == id_key(q) ||
            (!obj.name.empty() && key(obj.name) == q) || key(obj.label()) == q) {
            return &obj;
        }
    }
    return nullptr;
}

} // namespace zerosky::sky
