/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "calibration_fit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace zerosky::orientation {
namespace {

using Vec = std::array<double, 3>;
using Params = std::array<double, 9>; // centre xyz; Q xx,yy,zz,xy,xz,yz

Vec array(sensors::Vec3 v) { return {v.x, v.y, v.z}; }

double squared_distance(Vec a, Vec b) {
    double sum = 0;
    for (int i = 0; i < 3; ++i) sum += (a[i] - b[i]) * (a[i] - b[i]);
    return sum;
}

Matrix3 shape(const Params& p) {
    return {{{p[3], p[6], p[7]},
             {p[6], p[4], p[8]},
             {p[7], p[8], p[5]}}};
}

double determinant(const Matrix3& a) {
    return a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
           a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
           a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
}

bool inverse(const Matrix3& a, Matrix3& result) {
    const double d = determinant(a);
    if (!std::isfinite(d) || d <= 1e-12) return false;
    result = {{{(a[1][1] * a[2][2] - a[1][2] * a[2][1]) / d,
                (a[0][2] * a[2][1] - a[0][1] * a[2][2]) / d,
                (a[0][1] * a[1][2] - a[0][2] * a[1][1]) / d},
               {(a[1][2] * a[2][0] - a[1][0] * a[2][2]) / d,
                (a[0][0] * a[2][2] - a[0][2] * a[2][0]) / d,
                (a[0][2] * a[1][0] - a[0][0] * a[1][2]) / d},
               {(a[1][0] * a[2][1] - a[1][1] * a[2][0]) / d,
                (a[0][1] * a[2][0] - a[0][0] * a[2][1]) / d,
                (a[0][0] * a[1][1] - a[0][1] * a[1][0]) / d}}};
    return true;
}

bool diagonalize(Matrix3 a, Vec& eigenvalues, Matrix3& vectors) {
    vectors = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    for (int iteration = 0; iteration < 32; ++iteration) {
        int row = 0, col = 1;
        for (int i = 0; i < 3; ++i)
            for (int j = i + 1; j < 3; ++j)
                if (std::abs(a[i][j]) > std::abs(a[row][col])) {
                    row = i;
                    col = j;
                }
        if (std::abs(a[row][col]) < 1e-12) break;
        const double theta = 0.5 * std::atan2(2 * a[row][col],
                                               a[col][col] - a[row][row]);
        const double c = std::cos(theta), s = std::sin(theta);
        // Columns of vectors are eigenvectors. This is a Jacobi plane rotation.
        for (int k = 0; k < 3; ++k) {
            const double x = vectors[k][row], y = vectors[k][col];
            vectors[k][row] = c * x - s * y;
            vectors[k][col] = s * x + c * y;
        }
        const double app = a[row][row], aqq = a[col][col], apq = a[row][col];
        for (int k = 0; k < 3; ++k) {
            if (k == row || k == col) continue;
            const double x = a[k][row], y = a[k][col];
            a[k][row] = a[row][k] = c * x - s * y;
            a[k][col] = a[col][k] = s * x + c * y;
        }
        a[row][row] = c * c * app - 2 * c * s * apq + s * s * aqq;
        a[col][col] = s * s * app + 2 * c * s * apq + c * c * aqq;
        a[row][col] = a[col][row] = 0;
    }
    eigenvalues = {a[0][0], a[1][1], a[2][2]};
    return std::all_of(eigenvalues.begin(), eigenvalues.end(),
                       [](double x) { return std::isfinite(x) && x > 1e-9; });
}

bool solve(std::array<std::array<double, 9>, 9> a,
           std::array<double, 9> b, Params& x) {
    for (int col = 0; col < 9; ++col) {
        int pivot = col;
        for (int row = col + 1; row < 9; ++row)
            if (std::abs(a[row][col]) > std::abs(a[pivot][col])) pivot = row;
        if (std::abs(a[pivot][col]) < 1e-12) return false;
        std::swap(a[pivot], a[col]);
        std::swap(b[pivot], b[col]);
        const double scale = a[col][col];
        for (int j = col; j < 9; ++j) a[col][j] /= scale;
        b[col] /= scale;
        for (int row = 0; row < 9; ++row) {
            if (row == col) continue;
            const double factor = a[row][col];
            for (int j = col; j < 9; ++j) a[row][j] -= factor * a[col][j];
            b[row] -= factor * b[col];
        }
    }
    x = b;
    return true;
}

double residual(const Params& p, Vec point) {
    const Vec x{point[0] - p[0], point[1] - p[1], point[2] - p[2]};
    return p[3] * x[0] * x[0] + p[4] * x[1] * x[1] +
           p[5] * x[2] * x[2] + 2 * p[6] * x[0] * x[1] +
           2 * p[7] * x[0] * x[2] + 2 * p[8] * x[1] * x[2] - 1.0;
}

double loss(const Params& p, const std::vector<Vec>& training) {
    double sum = 0;
    for (Vec point : training) {
        const double r = residual(p, point);
        sum += r * r;
    }
    return sum;
}

bool fit(const std::vector<Vec>& training, Params& p) {
    Vec mean{};
    for (Vec point : training)
        for (int k = 0; k < 3; ++k) mean[k] += point[k];
    for (double& value : mean) value /= training.size();
    Matrix3 covariance{};
    for (Vec point : training)
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                covariance[i][j] += (point[i] - mean[i]) * (point[j] - mean[j]);
    for (auto& row : covariance)
        for (double& value : row) value /= training.size();
    Matrix3 inv{};
    if (!inverse(covariance, inv)) return false;
    p = {mean[0], mean[1], mean[2],
         inv[0][0] / 3, inv[1][1] / 3, inv[2][2] / 3,
         inv[0][1] / 3, inv[0][2] / 3, inv[1][2] / 3};

    for (int iteration = 0; iteration < 40; ++iteration) {
        std::array<std::array<double, 9>, 9> normal{};
        std::array<double, 9> rhs{};
        for (Vec point : training) {
            const Vec x{point[0] - p[0], point[1] - p[1], point[2] - p[2]};
            const Vec qx{p[3] * x[0] + p[6] * x[1] + p[7] * x[2],
                         p[6] * x[0] + p[4] * x[1] + p[8] * x[2],
                         p[7] * x[0] + p[8] * x[1] + p[5] * x[2]};
            const Params jacobian{-2 * qx[0], -2 * qx[1], -2 * qx[2],
                                  x[0] * x[0], x[1] * x[1], x[2] * x[2],
                                  2 * x[0] * x[1], 2 * x[0] * x[2],
                                  2 * x[1] * x[2]};
            const double r = residual(p, point);
            for (int i = 0; i < 9; ++i) {
                rhs[i] -= jacobian[i] * r;
                for (int j = 0; j < 9; ++j)
                    normal[i][j] += jacobian[i] * jacobian[j];
            }
        }
        for (int i = 0; i < 9; ++i) normal[i][i] += 1e-8 * training.size();
        Params step{};
        if (!solve(normal, rhs, step)) return false;
        const double old_loss = loss(p, training);
        bool accepted = false;
        for (double scale = 1.0; scale >= 1.0 / 1024.0; scale *= 0.5) {
            Params candidate = p;
            for (int i = 0; i < 9; ++i) candidate[i] += scale * step[i];
            Vec eigen{};
            Matrix3 axes{};
            if (diagonalize(shape(candidate), eigen, axes) &&
                loss(candidate, training) < old_loss) {
                p = candidate;
                accepted = true;
                break;
            }
        }
        if (!accepted) break;
    }
    Vec eigen{};
    Matrix3 axes{};
    return diagonalize(shape(p), eigen, axes);
}

bool orthogonal(const Matrix3& a) {
    if (!std::isfinite(determinant(a)) ||
        std::abs(std::abs(determinant(a)) - 1.0) > 0.02)
        return false;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            double dot = 0;
            for (int k = 0; k < 3; ++k) dot += a[k][i] * a[k][j];
            if (std::abs(dot - (i == j ? 1.0 : 0.0)) > 0.02) return false;
        }
    return true;
}

} // namespace

bool fit_magnetometer_calibration(const std::vector<CalibrationSample>& samples,
                                  const std::string& configuration,
                                  double expected_field_uT,
                                  const Matrix3& sensor_to_body,
                                  Calibration& result,
                                  std::string& error) {
    error.clear();
    if (configuration.empty() || !std::isfinite(expected_field_uT) ||
        expected_field_uT <= 0 || !orthogonal(sensor_to_body)) {
        error = "Invalid calibration configuration, field or sensor frame";
        return false;
    }
    std::vector<Vec> independent;
    independent.reserve(samples.size());
    for (const auto& sample : samples) {
        const Vec value = array(sample.raw_magnetic_uT);
        if (!std::isfinite(value[0]) || !std::isfinite(value[1]) ||
            !std::isfinite(value[2])) continue;
        // A fresh IIO read can repeat a quantized value. Count only moved poses.
        if (!independent.empty() && squared_distance(value, independent.back()) < 0.25)
            continue;
        independent.push_back(value);
    }
    if (independent.size() < 500) {
        error = "Calibration needs at least 500 independent samples";
        return false;
    }
    std::vector<Vec> training, held_out;
    training.reserve(independent.size());
    held_out.reserve(independent.size() / 5 + 1);
    for (size_t i = 0; i < independent.size(); ++i) {
        Vec normalized = independent[i];
        for (double& value : normalized) value /= expected_field_uT;
        (i % 5 == 0 ? held_out : training).push_back(normalized);
    }
    Params p{};
    if (!fit(training, p)) {
        error = "Magnetometer ellipsoid fit is degenerate";
        return false;
    }
    Vec eigen{};
    Matrix3 axes{};
    if (!diagonalize(shape(p), eigen, axes)) {
        error = "Magnetometer ellipsoid is not positive definite";
        return false;
    }
    const auto limits = std::minmax_element(eigen.begin(), eigen.end());
    if (std::sqrt(*limits.second / *limits.first) >= 4.0) {
        error = "Magnetometer ellipsoid axis ratio is too large";
        return false;
    }
    std::array<int, 8> octants{};
    for (Vec point : independent) {
        int octant = 0;
        for (int axis = 0; axis < 3; ++axis)
            if (point[axis] / expected_field_uT >= p[axis]) octant |= 1 << axis;
        ++octants[octant];
    }
    if (std::any_of(octants.begin(), octants.end(),
                    [](int count) { return count < 10; })) {
        error = "Calibration does not cover all eight octants";
        return false;
    }
    double held_error_sq = 0;
    for (Vec point : held_out) {
        const double radius_sq = residual(p, point) + 1.0;
        if (radius_sq < 0 || !std::isfinite(radius_sq)) {
            error = "Invalid held-out magnetic residual";
            return false;
        }
        const double residual_uT = expected_field_uT * (std::sqrt(radius_sq) - 1.0);
        held_error_sq += residual_uT * residual_uT;
    }
    const double rms = std::sqrt(held_error_sq / held_out.size());
    if (!std::isfinite(rms) || rms >= 5.0) {
        error = "Held-out magnetic residual exceeds 5 uT";
        return false;
    }
    Matrix3 soft{};
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            for (int k = 0; k < 3; ++k)
                soft[row][col] += axes[row][k] * std::sqrt(eigen[k]) * axes[col][k];
    result = Calibration{configuration,
                         {p[0] * expected_field_uT,
                          p[1] * expected_field_uT,
                          p[2] * expected_field_uT},
                         soft, sensor_to_body, expected_field_uT, rms,
                         static_cast<int>(independent.size())};
    return true;
}

} // namespace zerosky::orientation
