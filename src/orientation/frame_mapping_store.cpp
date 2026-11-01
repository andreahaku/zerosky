/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#include "frame_mapping_store.h"

#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <locale>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace zerosky::orientation {
namespace {

constexpr size_t kMaxFile = 4096;

class Reader {
public:
    explicit Reader(std::string_view text) : text_(text) {}

    bool mapping(FrameMapping& out) {
        if (!take('{')) return false;
        unsigned fields = 0;
        double schema = 0;
        do {
            std::string key;
            if (!string(key) || !take(':')) return false;
            unsigned bit = 0;
            if (key == "schema") {
                bit = 1;
                if (!number(schema) || (schema != 1.0 && schema != 2.0)) return false;
            } else if (key == "accel_to_body") {
                bit = 2;
                if (!matrix3(out.accel_to_body)) return false;
            } else if (key == "gyro_to_body") {
                bit = 4;
                if (!matrix3(out.gyro_to_body)) return false;
            } else if (key == "pointing_body") {
                bit = 8;
                if (!vector3(out.pointing_body)) return false;
            } else if (key == "screen_up_body") {
                bit = 16;
                if (!vector3(out.screen_up_body)) return false;
            } else if (key == "mag_to_body") {
                bit = 32;
                if (!matrix3(out.mag_to_body)) return false;
            } else {
                return false;
            }
            if (fields & bit) return false;
            fields |= bit;
            if (take('}')) break;
            if (!take(',')) return false;
        } while (true);
        spaces();
        out.mag_verified = schema == 2.0;
        return fields == (schema == 2.0 ? 63u : 31u) && at_ == text_.size();
    }

private:
    void spaces() {
        while (at_ < text_.size() &&
               (text_[at_] == ' ' || text_[at_] == '\t' ||
                text_[at_] == '\r' || text_[at_] == '\n')) ++at_;
    }

    bool take(char expected) {
        spaces();
        if (at_ >= text_.size() || text_[at_] != expected) return false;
        ++at_;
        return true;
    }

    bool string(std::string& out) {
        if (!take('"')) return false;
        out.clear();
        while (at_ < text_.size()) {
            const unsigned char c = static_cast<unsigned char>(text_[at_++]);
            if (c == '"') return true;
            if (c < 0x20 || c == '\\') return false; // fixed schema keys need no escapes
            out.push_back(static_cast<char>(c));
        }
        return false;
    }

    bool number(double& out) {
        spaces();
        const size_t begin = at_;
        if (at_ < text_.size() && text_[at_] == '-') ++at_;
        if (at_ == text_.size()) return false;
        if (text_[at_] == '0') {
            ++at_;
        } else {
            if (text_[at_] < '1' || text_[at_] > '9') return false;
            while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9') ++at_;
        }
        if (at_ < text_.size() && text_[at_] == '.') {
            ++at_;
            const size_t start = at_;
            while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9') ++at_;
            if (at_ == start) return false;
        }
        if (at_ < text_.size() && (text_[at_] == 'e' || text_[at_] == 'E')) {
            ++at_;
            if (at_ < text_.size() && (text_[at_] == '+' || text_[at_] == '-')) ++at_;
            const size_t start = at_;
            while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9') ++at_;
            if (at_ == start) return false;
        }
        std::istringstream stream(std::string(text_.substr(begin, at_ - begin)));
        stream.imbue(std::locale::classic());
        return static_cast<bool>(stream >> out) && std::isfinite(out);
    }

    bool vector3(sensors::Vec3& v) {
        return take('[') && number(v.x) && take(',') && number(v.y) &&
               take(',') && number(v.z) && take(']');
    }

    bool matrix3(Matrix3& m) {
        if (!take('[')) return false;
        for (size_t row = 0; row < 3; ++row) {
            if (row && !take(',')) return false;
            if (!take('[')) return false;
            for (size_t column = 0; column < 3; ++column) {
                if (column && !take(',')) return false;
                if (!number(m[row][column])) return false;
            }
            if (!take(']')) return false;
        }
        return take(']');
    }

    std::string_view text_;
    size_t at_{0};
};

double determinant(const Matrix3& a) {
    return a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
           a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
           a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
}

bool orthogonal(const Matrix3& a) {
    if (!std::isfinite(determinant(a)) || std::abs(std::abs(determinant(a)) - 1.0) > 1e-3)
        return false;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            double dot = 0.0;
            for (size_t k = 0; k < 3; ++k) dot += a[k][i] * a[k][j];
            if (!std::isfinite(dot) || std::abs(dot - (i == j ? 1.0 : 0.0)) > 1e-3)
                return false;
        }
    }
    return true;
}

bool rotation(const Matrix3& a) {
    return orthogonal(a) && determinant(a) > 0.0;
}

bool axes(sensors::Vec3 pointing, sensors::Vec3 screen_up) {
    const double pn = pointing.x * pointing.x + pointing.y * pointing.y +
                      pointing.z * pointing.z;
    const double un = screen_up.x * screen_up.x + screen_up.y * screen_up.y +
                      screen_up.z * screen_up.z;
    const double dot = pointing.x * screen_up.x + pointing.y * screen_up.y +
                       pointing.z * screen_up.z;
    return std::isfinite(pn) && std::isfinite(un) && std::isfinite(dot) &&
           std::abs(pn - 1.0) <= 1e-3 && std::abs(un - 1.0) <= 1e-3 &&
           std::abs(dot) <= 1e-3;
}

bool valid(const FrameMapping& mapping) {
    return rotation(mapping.accel_to_body) && rotation(mapping.gyro_to_body) &&
           axes(mapping.pointing_body, mapping.screen_up_body) &&
           (!mapping.mag_verified || orthogonal(mapping.mag_to_body));
}

void vector_json(std::ostream& out, sensors::Vec3 v) {
    out << '[' << v.x << ',' << v.y << ',' << v.z << ']';
}

void matrix_json(std::ostream& out, const Matrix3& matrix) {
    out << '[';
    for (size_t i = 0; i < 3; ++i) {
        if (i) out << ',';
        out << '[';
        for (size_t j = 0; j < 3; ++j) {
            if (j) out << ',';
            out << matrix[i][j];
        }
        out << ']';
    }
    out << ']';
}

bool write_all(int fd, const std::string& text) {
    size_t at = 0;
    while (at < text.size()) {
        const ssize_t count = ::write(fd, text.data() + at, text.size() - at);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        at += static_cast<size_t>(count);
    }
    return true;
}

} // namespace

std::filesystem::path default_frame_mapping_file() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg)
        return std::filesystem::path(xdg) / "zerosky/frame-mapping.json";
    if (const char* home = std::getenv("HOME"); home && *home)
        return std::filesystem::path(home) / ".config/zerosky/frame-mapping.json";
    return {};
}

bool load_frame_mapping(const std::filesystem::path& file,
                        FrameMapping& result, std::string& error) {
    result = FrameMapping{};
    error.clear();
    std::ifstream input(file, std::ios::binary);
    if (!input) { error = "Cannot open frame mapping"; return false; }
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    if (size < 0 || size > static_cast<std::streamoff>(kMaxFile)) {
        error = "Frame mapping is unreadable or too large";
        return false;
    }
    input.seekg(0, std::ios::beg);
    const std::string text((std::istreambuf_iterator<char>(input)),
                            std::istreambuf_iterator<char>());
    if (input.bad() || text.size() != static_cast<size_t>(size)) {
        error = "Frame mapping is unreadable or too large";
        return false;
    }
    FrameMapping loaded;
    if (!Reader(text).mapping(loaded) || !valid(loaded)) {
        error = "Invalid or unsupported frame mapping";
        return false;
    }
    loaded.verified = true;
    result = loaded;
    return true;
}

bool save_frame_mapping(const std::filesystem::path& file,
                        const FrameMapping& mapping, std::string& error) {
    error.clear();
    if (file.empty() || !mapping.verified || !mapping.mag_verified || !valid(mapping)) {
        error = "Frame mapping is unverified or invalid";
        return false;
    }
    std::error_code ec;
    if (std::filesystem::is_symlink(file, ec)) {
        error = "Frame mapping path cannot be a symbolic link";
        return false;
    }
    const bool existing = std::filesystem::exists(file, ec);
    const bool regular = existing && std::filesystem::is_regular_file(file, ec);
    if (ec || (existing && !regular)) {
        error = "Cannot inspect frame mapping file";
        return false;
    }
    if (existing) {
        FrameMapping previous;
        std::string load_error;
        if (!load_frame_mapping(file, previous, load_error)) {
            error = "Existing frame mapping is invalid: " + load_error;
            return false;
        }
    }

    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(17) << "{\"schema\":2,\"accel_to_body\":";
    matrix_json(stream, mapping.accel_to_body);
    stream << ",\"gyro_to_body\":";
    matrix_json(stream, mapping.gyro_to_body);
    stream << ",\"mag_to_body\":";
    matrix_json(stream, mapping.mag_to_body);
    stream << ",\"pointing_body\":";
    vector_json(stream, mapping.pointing_body);
    stream << ",\"screen_up_body\":";
    vector_json(stream, mapping.screen_up_body);
    stream << "}\n";
    const std::string output = stream.str();
    if (output.size() > kMaxFile) { error = "Frame mapping is too large"; return false; }

    const std::filesystem::path parent = file.parent_path().empty()
                                             ? std::filesystem::path(".")
                                             : file.parent_path();
    std::filesystem::create_directories(parent, ec);
    if (ec) { error = "Cannot create frame mapping directory"; return false; }
    static std::atomic<unsigned> sequence{0};
    const std::string suffix = ".tmp." + std::to_string(::getpid()) + "." +
                               std::to_string(sequence.fetch_add(1));
    const std::filesystem::path temporary = file.string() + suffix;
    const int fd = ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) { error = "Cannot create temporary frame mapping"; return false; }
    const bool written = write_all(fd, output) && ::fsync(fd) == 0;
    const bool closed = ::close(fd) == 0;
    if (!written || !closed) {
        std::filesystem::remove(temporary, ec);
        error = "Cannot write temporary frame mapping";
        return false;
    }
    if (existing) {
        const std::filesystem::path backup = file.string() + ".bak";
        const std::filesystem::path backup_temp = backup.string() + suffix;
        std::filesystem::copy_file(file, backup_temp, ec);
        if (!ec) {
            const int backup_fd = ::open(backup_temp.c_str(), O_RDONLY);
            if (backup_fd < 0 || ::fsync(backup_fd) != 0)
                ec = std::make_error_code(std::errc::io_error);
            if (backup_fd >= 0) ::close(backup_fd);
        }
        if (!ec) std::filesystem::rename(backup_temp, backup, ec);
        if (ec) {
            std::filesystem::remove(temporary, ec);
            std::filesystem::remove(backup_temp, ec);
            error = "Cannot back up previous frame mapping";
            return false;
        }
    }
    std::filesystem::rename(temporary, file, ec);
    if (ec) {
        std::filesystem::remove(temporary, ec);
        error = "Cannot replace frame mapping atomically";
        return false;
    }
    const int dir_fd = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY);
    if (dir_fd >= 0) {
        ::fsync(dir_fd);
        ::close(dir_fd);
    }
    return true;
}

} // namespace zerosky::orientation
