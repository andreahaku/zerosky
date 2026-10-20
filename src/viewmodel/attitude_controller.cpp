/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "attitude_controller.h"

#include "attitude_service.h"
#include "camera_from_attitude.h"
#include "frame_mapping_store.h"
#include "geomagnetic.h"
#include "persisted_state.h"
#include "sensor_source.h"

#include <ctime>

namespace zerosky {
namespace {

double decimal_year_now() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    gmtime_r(&now, &utc);
    return orientation::decimal_year_utc(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday)
        .value_or(0.0);
}

} // namespace

AttitudeController::AttitudeController() : mapping_(std::make_unique<orientation::FrameMapping>()) {
#if USE_DESKTOP
    unavailable_ = "manual";
#endif
}

AttitudeController::~AttitudeController() {
    if (service_) service_->stop();
}

void AttitudeController::start(const sky::Observer& observer, const std::string& hardware,
                               const std::filesystem::path& cof_file) {
#if USE_DESKTOP
    (void)observer;
    (void)hardware;
    (void)cof_file;
#else
    std::string error;
    if (!service_) {
        if (!orientation::load_frame_mapping(orientation::default_frame_mapping_file(), *mapping_,
                                             error)) {
            unavailable_ = "axes not measured";
            return;
        }
        auto source = std::make_unique<sensors::IioSource>();
        if (!source->discover(error)) {
            unavailable_ = "no IMU";
            return;
        }
        service_ = std::make_unique<orientation::AttitudeService>(std::move(source));
        if (!service_->set_frame_mapping(*mapping_, error)) {
            unavailable_ = "bad axes file";
            service_.reset();
            return;
        }
        // A missing profile leaves the compass unreliable; the status says so.
        service_->select_configuration(
            hardware, toolkit::config_file("zerosky", "magnetometer-calibration.json"), error);
        if (!service_->start(error)) {
            unavailable_ = "IMU did not start";
            service_.reset();
            return;
        }
    }
    // Loaded on any call, not only the first: a failed load is retried.
    if (!model_) model_ = orientation::GeomagneticModel::load(cof_file, error);
    if (model_) {
        service_->set_geomagnetic_location(*model_, observer.latitude_deg, observer.longitude_deg,
                                           observer.height_m, decimal_year_now(), error);
    }
    unavailable_.clear();
#endif
}

std::optional<sky::Camera> AttitudeController::camera(const sky::Camera& previous) const {
    if (!service_) return std::nullopt;
    return orientation::camera_from_attitude(service_->snapshot(), *mapping_, previous);
}

bool AttitudeController::compass_usable() const {
    // A disturbed compass keeps steering, and the status line warns.
    return service_ && service_->snapshot().compass != orientation::CompassStatus::Unreliable;
}

void AttitudeController::select_hardware(const std::string& hardware) {
    if (!service_) return;
    std::string error;
    service_->select_configuration(
        hardware, toolkit::config_file("zerosky", "magnetometer-calibration.json"), error);
}

std::string AttitudeController::status() const {
    if (!service_) return unavailable_;
    switch (service_->snapshot().compass) {
        case orientation::CompassStatus::Ok: return "";
        case orientation::CompassStatus::Disturbed: return "compass disturbed";
        case orientation::CompassStatus::Unreliable: return "tilt only";
    }
    return "";
}

} // namespace zerosky
