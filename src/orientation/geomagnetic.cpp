/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "geomagnetic.h"

extern "C" {
#include "GeomagnetismHeader.h"
}

#include <cmath>
#include <fstream>
#include <sstream>
#include <utility>

namespace zerosky::orientation {
namespace {

constexpr int kMaxDegree = 12;
constexpr int kTerms = kMaxDegree * (kMaxDegree + 3) / 2;

bool valid_cof(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::string line, name, release;
    double epoch = 0;
    if (!std::getline(input, line)) return false;
    std::istringstream header(line);
    if (!(header >> epoch >> name >> release) || epoch != 2025.0 || name != "WMM-2025")
        return false;
    for (int n = 1; n <= kMaxDegree; ++n) {
        for (int m = 0; m <= n; ++m) {
            if (!std::getline(input, line)) return false;
            std::istringstream row(line);
            int row_n = 0, row_m = 0;
            double g = 0, h = 0, dg = 0, dh = 0;
            if (!(row >> row_n >> row_m >> g >> h >> dg >> dh) ||
                row_n != n || row_m != m ||
                !std::isfinite(g) || !std::isfinite(h) ||
                !std::isfinite(dg) || !std::isfinite(dh)) return false;
        }
    }
    if (!std::getline(input, line)) return false;
    return line.find("9999") == 0;
}

bool leap_year(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

} // namespace

struct GeomagneticModel::Impl {
    MAGtype_MagneticModel* base = nullptr;
    MAGtype_Ellipsoid ellipsoid{};

    ~Impl() {
        if (base) MAG_FreeMagneticModelMemory(base);
    }
};

GeomagneticModel::GeomagneticModel(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
GeomagneticModel::~GeomagneticModel() = default;

std::unique_ptr<GeomagneticModel> GeomagneticModel::load(const std::filesystem::path& cof,
                                                         std::string& error) {
    error.clear();
    if (!valid_cof(cof)) {
        error = "Invalid or missing WMM2025 coefficient file";
        return nullptr;
    }
    auto impl = std::make_unique<Impl>();
    impl->base = MAG_AllocateModelMemory(kTerms);
    if (!impl->base) {
        error = "Cannot allocate WMM2025 model";
        return nullptr;
    }
    std::string path = cof.string();
    if (!MAG_readMagneticModel(path.data(), impl->base)) {
        error = "Cannot read WMM2025 coefficient file";
        return nullptr;
    }
    impl->base->nMax = kMaxDegree;
    impl->base->nMaxSecVar = kMaxDegree;
    MAGtype_Geoid unused_geoid{};
    MAG_SetDefaults(&impl->ellipsoid, &unused_geoid);
    return std::unique_ptr<GeomagneticModel>(new GeomagneticModel(std::move(impl)));
}

std::optional<GeomagneticField> GeomagneticModel::field(double latitude_deg,
                                                       double longitude_deg,
                                                       double height_wgs84_m,
                                                       double decimal_year) const {
    if (!std::isfinite(latitude_deg) || std::abs(latitude_deg) > 90.0 ||
        !std::isfinite(longitude_deg) || std::abs(longitude_deg) > 180.0 ||
        !std::isfinite(height_wgs84_m) || height_wgs84_m < -1000.0 ||
        height_wgs84_m > 850000.0 || !std::isfinite(decimal_year) ||
        decimal_year < 2025.0 || decimal_year >= 2030.0) return std::nullopt;

    MAGtype_MagneticModel* timed = MAG_AllocateModelMemory(kTerms);
    if (!timed) return std::nullopt;
    MAGtype_Date date{};
    date.DecimalYear = decimal_year;
    MAGtype_CoordGeodetic geodetic{};
    geodetic.phi = latitude_deg;
    geodetic.lambda = longitude_deg;
    geodetic.HeightAboveEllipsoid = height_wgs84_m / 1000.0;
    MAGtype_CoordSpherical spherical{};
    MAGtype_GeoMagneticElements elements{};
    const bool ok = MAG_TimelyModifyMagneticModel(date, impl_->base, timed) &&
                    MAG_GeodeticToSpherical(impl_->ellipsoid, geodetic, &spherical) &&
                    MAG_Geomag(impl_->ellipsoid, spherical, geodetic, timed, &elements);
    MAG_FreeMagneticModelMemory(timed);
    if (!ok || !std::isfinite(elements.X) || !std::isfinite(elements.Y) ||
        !std::isfinite(elements.Z) || !std::isfinite(elements.F) ||
        !std::isfinite(elements.Decl) || !std::isfinite(elements.Incl))
        return std::nullopt;
    return GeomagneticField{{elements.Y / 1000.0, elements.X / 1000.0,
                             -elements.Z / 1000.0},
                            elements.F / 1000.0, elements.Decl, elements.Incl};
}

std::optional<double> decimal_year_utc(int year, int month, int day) {
    if (year < 2025 || year > 2029 || month < 1 || month > 12) return std::nullopt;
    const int days[] = {31, leap_year(year) ? 29 : 28, 31, 30, 31, 30,
                        31, 31, 30, 31, 30, 31};
    if (day < 1 || day > days[month - 1]) return std::nullopt;
    int elapsed = day - 1;
    for (int m = 0; m < month - 1; ++m) elapsed += days[m];
    return year + static_cast<double>(elapsed) / (leap_year(year) ? 366.0 : 365.0);
}

} // namespace zerosky::orientation
