/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "sky_engine.h"

#include "astronomy.h"

#include <cmath>

namespace zerosky::sky {
namespace {

// Unix time 946728000 is 2000-01-01 12:00 UTC, the J2000 epoch Astronomy Engine counts from.
constexpr double kJ2000Unix = 946728000.0;

astro_time_t to_astro_time(double utc) {
    return Astronomy_TimeFromDays((utc - kJ2000Unix) / 86400.0);
}

astro_observer_t to_astro_observer(const Observer& observer) {
    return Astronomy_MakeObserver(observer.latitude_deg, observer.longitude_deg, observer.height_m);
}

struct BodyEntry {
    Body body;
    astro_body_t astro;
    const char* name;
};

constexpr BodyEntry kBodies[] = {
    {Body::Sun, BODY_SUN, "Sun"},
    {Body::Moon, BODY_MOON, "Moon"},
    {Body::Mercury, BODY_MERCURY, "Mercury"},
    {Body::Venus, BODY_VENUS, "Venus"},
    {Body::Mars, BODY_MARS, "Mars"},
    {Body::Jupiter, BODY_JUPITER, "Jupiter"},
    {Body::Saturn, BODY_SATURN, "Saturn"},
    {Body::Uranus, BODY_URANUS, "Uranus"},
    {Body::Neptune, BODY_NEPTUNE, "Neptune"},
};

} // namespace

std::vector<BodyPosition> solar_system(double utc, const Observer& observer) {
    const astro_observer_t where = to_astro_observer(observer);
    std::vector<BodyPosition> out;
    out.reserve(std::size(kBodies));
    for (const auto& entry : kBodies) {
        astro_time_t time = to_astro_time(utc);
        const astro_equatorial_t eq =
            Astronomy_Equator(entry.astro, &time, where, EQUATOR_OF_DATE, ABERRATION);
        if (eq.status != ASTRO_SUCCESS) continue;
        const astro_horizon_t hor =
            Astronomy_Horizon(&time, where, eq.ra, eq.dec, REFRACTION_NORMAL);

        BodyPosition pos;
        pos.body = entry.body;
        pos.name = entry.name;
        pos.horizontal = {hor.azimuth, hor.altitude};
        pos.ra_hours = eq.ra;
        pos.dec_deg = eq.dec;
        pos.distance_au = eq.dist;
        const astro_illum_t illum = Astronomy_Illumination(entry.astro, time);
        if (illum.status == ASTRO_SUCCESS) {
            pos.magnitude = illum.mag;
            pos.phase_fraction = illum.phase_fraction;
        }
        out.push_back(pos);
    }
    return out;
}

Horizontal star_horizontal(double ra_j2000_hours, double dec_j2000_deg, double utc,
                           const Observer& observer) {
    astro_time_t time = to_astro_time(utc);
    const astro_spherical_t sphere{ASTRO_SUCCESS, dec_j2000_deg, ra_j2000_hours * 15.0, 1.0};
    const astro_vector_t j2000 = Astronomy_VectorFromSphere(sphere, time);
    const astro_vector_t of_date = Astronomy_RotateVector(Astronomy_Rotation_EQJ_EQD(&time), j2000);
    const astro_equatorial_t eq = Astronomy_EquatorFromVector(of_date);
    if (eq.status != ASTRO_SUCCESS) return {NAN, NAN};
    const astro_horizon_t hor =
        Astronomy_Horizon(&time, to_astro_observer(observer), eq.ra, eq.dec, REFRACTION_NORMAL);
    return {hor.azimuth, hor.altitude};
}

} // namespace zerosky::sky
