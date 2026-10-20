/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "sensor_source.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void write(const std::filesystem::path& root, const std::string& name,
           const std::string& value) {
    std::ofstream file(root / name);
    require(static_cast<bool>(file), "Cannot create fake IIO sysfs file");
    file << value << '\n';
    require(file.good(), "Cannot write fake IIO sysfs file");
}

void close_to(double actual, double expected) {
    require(std::abs(actual - expected) < 1e-9, "Unexpected sensor conversion");
}

} // namespace

int main() {
    namespace fs = std::filesystem;
    using namespace zerosky::sensors;

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = fs::temp_directory_path() / ("zerosky-iio-test-" + std::to_string(stamp));
    fs::create_directories(root);
    const auto imu = root / "iio:device8";
    const auto mag = root / "iio:device1";
    fs::create_directories(imu);
    fs::create_directories(mag);

    write(imu, "name", "bmi270");
    write(imu, "in_accel_scale", "0.002394");
    write(imu, "in_anglvel_scale", "0.001065");
    write(imu, "in_accel_x_offset", "1");
    write(imu, "in_accel_x_raw", "9");
    write(imu, "in_accel_y_raw", "0");
    write(imu, "in_accel_z_raw", "100");
    write(imu, "in_anglvel_x_raw", "10");
    write(imu, "in_anglvel_y_raw", "20");
    write(imu, "in_anglvel_z_raw", "30");

    write(mag, "name", "bmc150_magn_bmi270");
    write(mag, "in_magn_scale", "0.000625");
    write(mag, "in_magn_x_raw", "16");
    write(mag, "in_magn_y_raw", "32");
    write(mag, "in_magn_z_raw", "48");
    write(mag, "in_mount_matrix", "1,0,0;0,1,0;0,0,1");

    IioSource reader(root);
    std::string error;
    require(reader.discover(error), error);
    require(reader.info().magnetic_present, "Magnetometer not discovered");
    require(reader.info().accelerometer_gyro_name == "bmi270", "Wrong BMI270");
    require(reader.info().magnetometer_mount_matrix == "1,0,0;0,1,0;0,0,1",
            "Mount matrix metadata lost");

    Sample sample;
    require(reader.read(sample, error), error);
    close_to(sample.acceleration_mps2.x, 10.0 * 0.002394);
    close_to(sample.acceleration_mps2.z, 100.0 * 0.002394);
    close_to(sample.angular_velocity_radps.y, 20.0 * 0.001065);
    require(sample.magnetic_present && sample.magnetic_fresh, "Magnetometer not fresh");
    close_to(sample.magnetic_uT.x, 1.0);
    close_to(sample.magnetic_uT.y, 2.0);
    close_to(sample.magnetic_uT.z, 3.0);

    Sample second;
    require(reader.read(second, error), error);
    require(second.magnetic_present && !second.magnetic_fresh, "Magnetometer overpolled");

    fs::remove_all(mag);
    require(reader.discover(error), error);
    require(!reader.info().magnetic_present, "Missing magnetometer still present");
    require(reader.read(sample, error), error);
    require(!sample.magnetic_present, "Six-axis sample marked magnetic");

    SimulatedSource simulation;
    require(simulation.simulated(), "Simulation mislabeled");
    require(simulation.read(sample, error), error);
    require(sample.magnetic_present && sample.magnetic_fresh, "Simulation sample missing");

    fs::remove_all(root);
}
