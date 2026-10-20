/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "attitude_service.h"
#include "calibration_fit.h"
#include "geomagnetic.h"
#include "frame_mapping_store.h"
#include "sensor_source.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

namespace {

using zerosky::orientation::AttitudeService;
using zerosky::orientation::Calibration;
using zerosky::orientation::CalibrationMetadata;
using zerosky::orientation::CompassStatus;
using zerosky::orientation::FrameMapping;
using zerosky::orientation::GeomagneticModel;
using zerosky::orientation::Matrix3;
using zerosky::sensors::Vec3;

volatile std::sig_atomic_t interrupted = 0;

void stop_on_signal(int) { interrupted = 1; }

struct Options {
    std::filesystem::path iio_root = "/sys/bus/iio/devices";
    std::filesystem::path calibration_file;
    std::filesystem::path frame_mapping_file;
    std::filesystem::path cof_file;
    std::string hardware = "none";
    bool capture = false;
    bool save_frames = false;
    bool have_lat = false;
    bool have_lon = false;
    bool have_date = false;
    bool have_mag_map = false;
    bool have_accel_map = false;
    bool have_gyro_map = false;
    bool have_pointing = false;
    bool have_screen_up = false;
    double latitude = 0;
    double longitude = 0;
    double height_m = 0;
    double decimal_year = 0;
    int seconds = 0;
    Matrix3 mag_to_body{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    FrameMapping frame;
};

void usage(std::ostream& out) {
    out << "Usage: zerosky_probe [options]\n"
           "  --hardware ID           Magnetic hardware profile (none, cap, usb_gps, ...)\n"
           "  --lat DEG --lon DEG --date YYYY-MM-DD  WMM position and UTC day\n"
           "  --height-m METRES       WGS84 ellipsoid height (default 0)\n"
           "  --capture ID            Collect for 120 seconds, fit and save this profile\n"
           "  --seconds N             Capture length (30-300), or monitor length (0=until Ctrl-C)\n"
           "  --mag-to-body 9CSV      Verified BMM150-to-body rotation; required for capture\n"
           "  --accel-to-body 9CSV    Verified BMI270 acceleration rotation\n"
           "  --gyro-to-body 9CSV     Verified BMI270 gyro rotation\n"
           "  --pointing-body 3CSV    Verified device pointing axis\n"
           "  --screen-up-body 3CSV   Verified top edge of the display\n"
           "  --save-frames           Validate and save frame axes, then exit\n"
           "  --frame-mapping FILE    Frame JSON (default XDG config path)\n"
           "  --calibration FILE      Profile JSON (default XDG config path)\n"
           "  --cof FILE              WMM2025 coefficients\n"
           "  --iio-root DIR          IIO sysfs root for diagnostics\n"
           "  --help                  Show this help\n"
           "The four frame options must be supplied together before az/alt can be\n"
           "treated as verified. Capture requires --mag-to-body even if it is identity.\n";
}

bool number(const std::string& text, double& value) {
    try {
        size_t used = 0;
        value = std::stod(text, &used);
        return used == text.size() && std::isfinite(value);
    } catch (...) { return false; }
}

bool integer(const std::string& text, int& value) {
    try {
        size_t used = 0;
        value = std::stoi(text, &used);
        return used == text.size();
    } catch (...) { return false; }
}

bool vector_csv(const std::string& text, Vec3& result) {
    std::istringstream stream(text);
    std::string part;
    double values[3]{};
    for (double& value : values) {
        if (!std::getline(stream, part, ',') || !number(part, value)) return false;
    }
    if (std::getline(stream, part, ',')) return false;
    result = {values[0], values[1], values[2]};
    return true;
}

bool matrix_csv(const std::string& text, Matrix3& result) {
    std::istringstream stream(text);
    std::string part;
    for (auto& row : result)
        for (double& value : row)
            if (!std::getline(stream, part, ',') || !number(part, value)) return false;
    return !std::getline(stream, part, ',');
}

bool utc_day(const std::string& text, double& decimal_year) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') return false;
    for (size_t i = 0; i < text.size(); ++i)
        if (i != 4 && i != 7 && (text[i] < '0' || text[i] > '9')) return false;
    const int year = std::stoi(text.substr(0, 4));
    const int month = std::stoi(text.substr(5, 2));
    const int day = std::stoi(text.substr(8, 2));
    const auto parsed = zerosky::orientation::decimal_year_utc(year, month, day);
    if (!parsed) return false;
    decimal_year = *parsed;
    return true;
}

std::filesystem::path default_calibration_file() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg)
        return std::filesystem::path(xdg) / "zerosky/magnetometer-calibration.json";
    if (const char* home = std::getenv("HOME"); home && *home)
        return std::filesystem::path(home) / ".config/zerosky/magnetometer-calibration.json";
    return {};
}

std::filesystem::path default_cof_file() {
    const std::filesystem::path installed = "/usr/share/zerosky/wmm/WMM.COF";
    if (std::filesystem::exists(installed)) return installed;
    return "third_party/wmm/WMM.COF";
}

bool parse(int argc, char** argv, Options& options, std::string& error) {
    options.calibration_file = default_calibration_file();
    options.frame_mapping_file = zerosky::orientation::default_frame_mapping_file();
    options.cof_file = default_cof_file();
    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];
        if (flag == "--help") { usage(std::cout); std::exit(0); }
        if (flag == "--save-frames") { options.save_frames = true; continue; }
        if (i + 1 >= argc) { error = "Missing value for " + flag; return false; }
        const std::string value = argv[++i];
        bool ok = true;
        if (flag == "--hardware") options.hardware = value;
        else if (flag == "--capture") { options.capture = true; options.hardware = value; }
        else if (flag == "--lat") { options.have_lat = true; ok = number(value, options.latitude); }
        else if (flag == "--lon") { options.have_lon = true; ok = number(value, options.longitude); }
        else if (flag == "--height-m") ok = number(value, options.height_m);
        else if (flag == "--date") { options.have_date = true; ok = utc_day(value, options.decimal_year); }
        else if (flag == "--seconds") ok = integer(value, options.seconds);
        else if (flag == "--mag-to-body") { options.have_mag_map = true; ok = matrix_csv(value, options.mag_to_body); }
        else if (flag == "--accel-to-body") { options.have_accel_map = true; ok = matrix_csv(value, options.frame.accel_to_body); }
        else if (flag == "--gyro-to-body") { options.have_gyro_map = true; ok = matrix_csv(value, options.frame.gyro_to_body); }
        else if (flag == "--pointing-body") { options.have_pointing = true; ok = vector_csv(value, options.frame.pointing_body); }
        else if (flag == "--screen-up-body") { options.have_screen_up = true; ok = vector_csv(value, options.frame.screen_up_body); }
        else if (flag == "--calibration") options.calibration_file = value;
        else if (flag == "--frame-mapping") options.frame_mapping_file = value;
        else if (flag == "--cof") options.cof_file = value;
        else if (flag == "--iio-root") options.iio_root = value;
        else { error = "Unknown option: " + flag; return false; }
        if (!ok) { error = "Invalid value for " + flag; return false; }
    }
    const bool have_position = options.have_lat || options.have_lon || options.have_date;
    if (have_position != (options.have_lat && options.have_lon && options.have_date)) {
        error = "--lat, --lon and --date must be supplied together";
        return false;
    }
    if (options.latitude < -90 || options.latitude > 90 ||
        options.longitude < -180 || options.longitude > 180 ||
        options.height_m < -1000 || options.height_m > 850000 ||
        options.hardware.empty() ||
        (!options.save_frames && options.calibration_file.empty())) {
        error = "Invalid position, hardware or calibration path";
        return false;
    }
    const bool any_frame = options.have_accel_map || options.have_gyro_map ||
                           options.have_pointing || options.have_screen_up;
    const bool full_frame = options.have_accel_map && options.have_gyro_map &&
                            options.have_pointing && options.have_screen_up;
    if (any_frame && !full_frame) {
        error = "Verified frame needs all four frame options";
        return false;
    }
    options.frame.verified = full_frame;
    if (options.save_frames && (options.capture || !full_frame ||
                                options.frame_mapping_file.empty())) {
        error = "--save-frames needs all four frame options and a path, without --capture";
        return false;
    }
    if (options.capture) {
        if (!have_position || !options.have_mag_map) {
            error = "Capture needs position, UTC date and verified --mag-to-body";
            return false;
        }
        if (options.seconds == 0) options.seconds = 120;
        if (options.seconds < 30 || options.seconds > 300) {
            error = "Capture duration must be 30-300 seconds";
            return false;
        }
    } else if (options.seconds < 0) {
        error = "Monitor duration cannot be negative";
        return false;
    }
    return true;
}

const char* compass_name(CompassStatus status) {
    switch (status) {
        case CompassStatus::Ok: return "ok";
        case CompassStatus::Disturbed: return "disturbed";
        case CompassStatus::Unreliable: return "unreliable";
    }
    return "unknown";
}

void print_snapshot(const zerosky::orientation::AttitudeSnapshot& snapshot,
                    bool frame_verified) {
    const auto& raw = snapshot.raw_sample;
    const auto& q = snapshot.body_to_enu;
    std::cout << std::fixed << std::setprecision(3)
              << "acc[m/s2] " << raw.acceleration_mps2.x << ','
              << raw.acceleration_mps2.y << ',' << raw.acceleration_mps2.z
              << " gyro[rad/s] " << raw.angular_velocity_radps.x << ','
              << raw.angular_velocity_radps.y << ',' << raw.angular_velocity_radps.z;
    if (snapshot.last_raw_magnetic_at != std::chrono::steady_clock::time_point{}) {
        const auto age = std::chrono::duration<double>(raw.sampled_at -
                                                       snapshot.last_raw_magnetic_at).count();
        std::cout << " mag[uT] " << snapshot.last_raw_magnetic_uT.x << ','
                  << snapshot.last_raw_magnetic_uT.y << ','
                  << snapshot.last_raw_magnetic_uT.z << " age[s] " << age;
    } else {
        std::cout << " mag[uT] unavailable";
    }
    std::cout << " q[wxyz] " << q.w << ',' << q.x << ',' << q.y << ',' << q.z
              << (frame_verified ? " point[az,alt] " : " point[az,alt,unverified] ")
              << snapshot.pointing_azimuth_deg << ','
              << snapshot.pointing_altitude_deg
              << " compass " << compass_name(snapshot.compass);
    if (!snapshot.error.empty()) std::cout << " error " << snapshot.error;
    std::cout << '\n';
}

std::string utc_timestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    if (!::gmtime_r(&now, &utc)) return {};
    char buffer[32]{};
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0)
        return {};
    return buffer;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    std::string error;
    if (!parse(argc, argv, options, error)) {
        std::cerr << error << '\n';
        usage(std::cerr);
        return 2;
    }
    if (options.save_frames) {
        if (!zerosky::orientation::save_frame_mapping(options.frame_mapping_file,
                                                       options.frame, error)) {
            std::cerr << "Frame mapping not saved: " << error << '\n';
            return 1;
        }
        std::cout << "Saved verified frame mapping to " << options.frame_mapping_file << '\n';
        return 0;
    }
    auto source = std::make_unique<zerosky::sensors::IioSource>(options.iio_root);
    if (!source->discover(error)) { std::cerr << error << '\n'; return 1; }
    const auto info = source->info();
    std::cout << "IIO: " << info.accelerometer_gyro_name
              << ", magnetometer: " << (info.magnetic_present ? info.magnetometer_name : "missing")
              << ", mount matrix: " << info.magnetometer_mount_matrix << '\n';
    if (options.capture && !info.magnetic_present) {
        std::cerr << "Capture requires BMM150 magnetic samples\n";
        return 1;
    }
    AttitudeService service(std::move(source));
    if (!service.set_frame_mapping(options.frame, error)) {
        std::cerr << error << '\n'; return 1;
    }
    if (!service.select_configuration(options.hardware, options.calibration_file, error))
        std::cerr << "Calibration: " << error << '\n';
    std::unique_ptr<GeomagneticModel> wmm;
    std::optional<zerosky::orientation::GeomagneticField> field;
    if (options.have_lat) {
        wmm = GeomagneticModel::load(options.cof_file, error);
        if (!wmm) { std::cerr << error << '\n'; return 1; }
        field = wmm->field(options.latitude, options.longitude,
                           options.height_m, options.decimal_year);
        if (!field || !service.set_geomagnetic_location(*wmm, options.latitude,
                options.longitude, options.height_m, options.decimal_year, error)) {
            std::cerr << "Cannot calculate WMM reference: " << error << '\n';
            return 1;
        }
        std::cout << "WMM2025: D=" << field->declination_deg << " deg east, F="
                  << field->total_uT << " uT\n";
    }
    if (!service.start(error)) { std::cerr << error << '\n'; return 1; }
    if (options.capture && !service.begin_calibration_capture(options.hardware, error)) {
        std::cerr << error << '\n'; service.stop(); return 1;
    }
    std::signal(SIGINT, stop_on_signal);
    std::signal(SIGTERM, stop_on_signal);
    const auto started = std::chrono::steady_clock::now();
    auto next = started;
    while (!interrupted) {
        const auto now = std::chrono::steady_clock::now();
        if (options.seconds > 0 && now - started >= std::chrono::seconds(options.seconds)) break;
        const auto snapshot = service.snapshot();
        if (snapshot.sampled_at != std::chrono::steady_clock::time_point{})
            print_snapshot(snapshot, options.frame.verified);
        next += std::chrono::milliseconds(200);
        std::this_thread::sleep_until(next);
    }
    std::vector<zerosky::orientation::CalibrationSample> captured;
    if (options.capture) captured = service.end_calibration_capture();
    service.stop();
    if (!options.capture) return 0;
    std::cout << "Captured " << captured.size() << " fresh magnetic samples\n";
    Calibration fitted;
    if (!zerosky::orientation::fit_magnetometer_calibration(
            captured, options.hardware, field->total_uT,
            options.mag_to_body, fitted, error)) {
        std::cerr << "Fit rejected: " << error << '\n';
        return 1;
    }
    const CalibrationMetadata metadata{utc_timestamp(), options.latitude, options.longitude};
    if (!zerosky::orientation::save_calibration(options.calibration_file,
                                                fitted, metadata, error)) {
        std::cerr << "Profile not saved: " << error << '\n';
        return 1;
    }
    std::cout << "Saved " << options.hardware << " to " << options.calibration_file
              << "; samples=" << fitted.samples << " held-out RMS="
              << fitted.rms_uT << " uT\n";
    return 0;
}
