/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "calibration.h"

#include <cmath>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <locale>
#include <map>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

namespace zerosky::orientation {
namespace {

struct Json {
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json>;
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;
};

// Small, strict reader for the profile schema. No external JSON dependency on
// the CardputerZero; unsupported escapes or malformed documents fail closed.
class JsonReader {
public:
    explicit JsonReader(std::string_view text) : text_(text) {}

    bool parse(Json& value) {
        if (!parse_value(value, 0)) return false;
        spaces();
        return at_ == text_.size();
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
            if (c < 0x20) return false;
            if (c != '\\') {
                out.push_back(static_cast<char>(c));
                continue;
            }
            if (at_ == text_.size()) return false;
            switch (text_[at_++]) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default: return false;
            }
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
            const size_t digits = at_;
            while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9') ++at_;
            if (at_ == digits) return false;
        }
        if (at_ < text_.size() && (text_[at_] == 'e' || text_[at_] == 'E')) {
            ++at_;
            if (at_ < text_.size() && (text_[at_] == '+' || text_[at_] == '-')) ++at_;
            const size_t digits = at_;
            while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9') ++at_;
            if (at_ == digits) return false;
        }
        std::istringstream stream(std::string(text_.substr(begin, at_ - begin)));
        stream.imbue(std::locale::classic());
        return static_cast<bool>(stream >> out) && std::isfinite(out);
    }

    bool parse_value(Json& out, unsigned depth) {
        if (depth > 16) return false;
        spaces();
        if (at_ >= text_.size()) return false;
        const char c = text_[at_];
        if (c == '"') {
            std::string value;
            if (!string(value)) return false;
            out.value = std::move(value);
            return true;
        }
        if (c == '{') {
            ++at_;
            Json::Object object;
            if (take('}')) { out.value = std::move(object); return true; }
            do {
                std::string key;
                if (!string(key) || !take(':')) return false;
                Json value;
                if (!parse_value(value, depth + 1)) return false;
                if (!object.emplace(std::move(key), std::move(value)).second) return false;
                if (take('}')) { out.value = std::move(object); return true; }
            } while (take(','));
            return false;
        }
        if (c == '[') {
            ++at_;
            Json::Array array;
            if (take(']')) { out.value = std::move(array); return true; }
            do {
                Json value;
                if (!parse_value(value, depth + 1)) return false;
                array.push_back(std::move(value));
                if (take(']')) { out.value = std::move(array); return true; }
            } while (take(','));
            return false;
        }
        for (const auto& literal : {std::pair{"true", Json{true}},
                                    std::pair{"false", Json{false}},
                                    std::pair{"null", Json{nullptr}}}) {
            const std::string_view word(literal.first);
            if (text_.substr(at_, word.size()) == word) {
                at_ += word.size();
                out = literal.second;
                return true;
            }
        }
        double value = 0.0;
        if (!number(value)) return false;
        out.value = value;
        return true;
    }

    std::string_view text_;
    size_t at_ = 0;
};

const Json* member(const Json& parent, const std::string& name) {
    const auto* object = std::get_if<Json::Object>(&parent.value);
    if (!object) return nullptr;
    const auto it = object->find(name);
    return it == object->end() ? nullptr : &it->second;
}

bool numeric(const Json* node, double& value) {
    if (!node) return false;
    const auto* number = std::get_if<double>(&node->value);
    if (!number || !std::isfinite(*number)) return false;
    value = *number;
    return true;
}

bool string_equal(const Json* node, const std::string& expected) {
    if (!node) return false;
    const auto* value = std::get_if<std::string>(&node->value);
    return value && *value == expected;
}

bool vector3(const Json* node, sensors::Vec3& out) {
    if (!node) return false;
    const auto* array = std::get_if<Json::Array>(&node->value);
    return array && array->size() == 3 &&
           numeric(&(*array)[0], out.x) && numeric(&(*array)[1], out.y) &&
           numeric(&(*array)[2], out.z);
}

bool matrix3(const Json* node, Matrix3& out) {
    if (!node) return false;
    const auto* rows = std::get_if<Json::Array>(&node->value);
    if (!rows || rows->size() != 3) return false;
    for (size_t i = 0; i < 3; ++i) {
        const auto* columns = std::get_if<Json::Array>(&(*rows)[i].value);
        if (!columns || columns->size() != 3) return false;
        for (size_t j = 0; j < 3; ++j) {
            if (!numeric(&(*columns)[j], out[i][j])) return false;
        }
    }
    return true;
}

double determinant(const Matrix3& a) {
    return a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
           a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
           a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
}

bool positive_definite(const Matrix3& a) {
    constexpr double eps = 1e-9;
    for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 3; ++j)
            if (std::abs(a[i][j] - a[j][i]) > 1e-6) return false;
    return a[0][0] > eps &&
           a[0][0] * a[1][1] - a[0][1] * a[1][0] > eps &&
           determinant(a) > eps;
}

// The BMM150 axes may form a left-handed triad relative to the BMI270.
// A magnetometer-to-body map can therefore be an orthogonal reflection.
bool orthogonal_matrix(const Matrix3& a) {
    if (!std::isfinite(determinant(a)) ||
        std::abs(std::abs(determinant(a)) - 1.0) > 0.02) return false;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            double dot = 0.0;
            for (size_t k = 0; k < 3; ++k) dot += a[k][i] * a[k][j];
            if (std::abs(dot - (i == j ? 1.0 : 0.0)) > 0.02) return false;
        }
    }
    return true;
}

sensors::Vec3 multiply(const Matrix3& a, sensors::Vec3 v) {
    return {a[0][0] * v.x + a[0][1] * v.y + a[0][2] * v.z,
            a[1][0] * v.x + a[1][1] * v.y + a[1][2] * v.z,
            a[2][0] * v.x + a[2][1] * v.y + a[2][2] * v.z};
}

Json vector_json(sensors::Vec3 v) {
    Json::Array values;
    values.emplace_back(Json{v.x});
    values.emplace_back(Json{v.y});
    values.emplace_back(Json{v.z});
    return Json{std::move(values)};
}

Json location_json(double latitude, double longitude) {
    Json::Array values;
    values.emplace_back(Json{latitude});
    values.emplace_back(Json{longitude});
    return Json{std::move(values)};
}

Json matrix_json(const Matrix3& matrix) {
    Json::Array rows;
    for (const auto& row : matrix) {
        Json::Array columns;
        for (double value : row) columns.emplace_back(Json{value});
        rows.emplace_back(Json{std::move(columns)});
    }
    return Json{std::move(rows)};
}

void write_json(std::ostream& out, const Json& json) {
    if (std::holds_alternative<std::nullptr_t>(json.value)) {
        out << "null";
    } else if (const auto* value = std::get_if<bool>(&json.value)) {
        out << (*value ? "true" : "false");
    } else if (const auto* value = std::get_if<double>(&json.value)) {
        out << *value;
    } else if (const auto* value = std::get_if<std::string>(&json.value)) {
        out << '"';
        for (char c : *value) {
            switch (c) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default: out << c;
            }
        }
        out << '"';
    } else if (const auto* value = std::get_if<Json::Array>(&json.value)) {
        out << '[';
        for (size_t i = 0; i < value->size(); ++i) {
            if (i) out << ',';
            write_json(out, (*value)[i]);
        }
        out << ']';
    } else if (const auto* value = std::get_if<Json::Object>(&json.value)) {
        out << '{';
        bool first = true;
        for (const auto& item : *value) {
            if (!first) out << ',';
            first = false;
            write_json(out, Json{item.first});
            out << ':';
            write_json(out, item.second);
        }
        out << '}';
    }
}

bool write_all(int fd, const std::string& text) {
    size_t position = 0;
    while (position < text.size()) {
        const ssize_t count = ::write(fd, text.data() + position, text.size() - position);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        position += static_cast<size_t>(count);
    }
    return true;
}

bool valid_configuration(const std::string& configuration) {
    if (configuration.empty() || configuration.size() > 64) return false;
    for (unsigned char c : configuration)
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') && c != '_' && c != '-') return false;
    return true;
}

} // namespace

bool load_calibration(const std::filesystem::path& file,
                      const std::string& configuration,
                      Calibration& result, std::string& error) {
    error.clear();
    std::ifstream input(file, std::ios::binary);
    if (!input) { error = "Cannot open magnetometer calibration"; return false; }
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    if (size < 0 || size > 64 * 1024) {
        error = "Cannot read magnetometer calibration or file is too large";
        return false;
    }
    input.seekg(0, std::ios::beg);
    std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (input.bad() || text.size() != static_cast<size_t>(size)) {
        error = "Cannot read magnetometer calibration or file is too large";
        return false;
    }
    Json root;
    if (!JsonReader(text).parse(root)) {
        error = "Invalid magnetometer calibration JSON";
        return false;
    }
    double schema = 0.0;
    if (!numeric(member(root, "schema"), schema) || schema != 1.0 ||
        !string_equal(member(root, "units"), "uT") ||
        !string_equal(member(root, "sensor"), "bmm150")) {
        error = "Unsupported magnetometer calibration schema, units or sensor";
        return false;
    }
    const Json* profiles = member(root, "profiles");
    const Json* profile = profiles ? member(*profiles, configuration) : nullptr;
    if (!profile || !std::get_if<Json::Object>(&profile->value)) {
        error = "No calibration profile for hardware configuration: " + configuration;
        return false;
    }
    Calibration loaded;
    loaded.configuration = configuration;
    double count = 0.0;
    if (!vector3(member(*profile, "offset_uT"), loaded.offset_uT) ||
        !matrix3(member(*profile, "soft_iron"), loaded.soft_iron) ||
        !matrix3(member(root, "sensor_to_body"), loaded.sensor_to_body) ||
        !numeric(member(*profile, "field_uT"), loaded.field_uT) ||
        !numeric(member(*profile, "rms_uT"), loaded.rms_uT) ||
        !numeric(member(*profile, "samples"), count) ||
        count < 1.0 || count > 10000000.0 || std::floor(count) != count ||
        loaded.field_uT <= 0.0 || loaded.rms_uT < 0.0 ||
        !positive_definite(loaded.soft_iron) ||
        !orthogonal_matrix(loaded.sensor_to_body)) {
        error = "Invalid magnetometer calibration values";
        return false;
    }
    loaded.samples = static_cast<int>(count);
    result = std::move(loaded);
    return true;
}

bool save_calibration(const std::filesystem::path& file,
                      const Calibration& calibration,
                      const CalibrationMetadata& metadata,
                      std::string& error) {
    error.clear();
    const auto finite = [](sensors::Vec3 v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };
    if (!valid_configuration(calibration.configuration) ||
        !finite(calibration.offset_uT) ||
        !positive_definite(calibration.soft_iron) ||
        !orthogonal_matrix(calibration.sensor_to_body) ||
        !std::isfinite(calibration.field_uT) || calibration.field_uT <= 0 ||
        !std::isfinite(calibration.rms_uT) || calibration.rms_uT >= 5.0 ||
        calibration.samples < 500 ||
        !std::isfinite(metadata.latitude_deg) ||
        std::abs(metadata.latitude_deg) > 90 ||
        !std::isfinite(metadata.longitude_deg) ||
        std::abs(metadata.longitude_deg) > 180 ||
        metadata.measured_at_utc.empty() ||
        metadata.measured_at_utc.size() > 32 ||
        metadata.measured_at_utc.back() != 'Z') {
        error = "Invalid calibration or measurement metadata";
        return false;
    }
    std::error_code ec;
    if (std::filesystem::is_symlink(file, ec)) {
        error = "Calibration path cannot be a symbolic link";
        return false;
    }
    const bool existing = std::filesystem::exists(file, ec);
    if (ec) { error = "Cannot inspect calibration file"; return false; }
    Json root;
    if (existing) {
        std::ifstream input(file, std::ios::binary);
        if (!input) { error = "Cannot read existing calibration"; return false; }
        const std::string contents((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());
        if (input.bad() || contents.size() > 64 * 1024 ||
            !JsonReader(contents).parse(root)) {
            error = "Existing calibration is invalid or too large";
            return false;
        }
        double schema = 0;
        Matrix3 previous_frame{};
        if (!numeric(member(root, "schema"), schema) || schema != 1.0 ||
            !string_equal(member(root, "units"), "uT") ||
            !string_equal(member(root, "sensor"), "bmm150") ||
            !matrix3(member(root, "sensor_to_body"), previous_frame) ||
            !orthogonal_matrix(previous_frame) ||
            !member(root, "profiles") ||
            !std::get_if<Json::Object>(&member(root, "profiles")->value)) {
            error = "Existing calibration has an incompatible schema";
            return false;
        }
        for (size_t i = 0; i < 3; ++i)
            for (size_t j = 0; j < 3; ++j)
                if (std::abs(previous_frame[i][j] -
                             calibration.sensor_to_body[i][j]) > 1e-6) {
                    error = "Sensor frame differs from existing calibration profiles";
                    return false;
                }
    } else {
        root.value = Json::Object{{"schema", Json{1.0}},
                                  {"units", Json{std::string("uT")}},
                                  {"sensor", Json{std::string("bmm150")}},
                                  {"sensor_to_body", matrix_json(calibration.sensor_to_body)},
                                  {"profiles", Json{Json::Object{}}}};
    }
    Json::Object profile{{"offset_uT", vector_json(calibration.offset_uT)},
                         {"soft_iron", matrix_json(calibration.soft_iron)},
                         {"field_uT", Json{calibration.field_uT}},
                         {"rms_uT", Json{calibration.rms_uT}},
                         {"samples", Json{static_cast<double>(calibration.samples)}},
                         {"measured_at_utc", Json{metadata.measured_at_utc}},
                         {"location_deg", location_json(metadata.latitude_deg,
                                                          metadata.longitude_deg)},
                         {"wmm_model", Json{std::string("WMM2025")}}};
    auto* profiles = std::get_if<Json::Object>(&
        std::get<Json::Object>(root.value).at("profiles").value);
    (*profiles)[calibration.configuration] = Json{std::move(profile)};
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(17);
    write_json(stream, root);
    stream << '\n';
    const std::string output = stream.str();
    if (output.size() > 64 * 1024) {
        error = "Calibration file would exceed 64 KiB";
        return false;
    }
    const std::filesystem::path parent = file.parent_path().empty()
                                             ? std::filesystem::path(".")
                                             : file.parent_path();
    std::filesystem::create_directories(parent, ec);
    if (ec) { error = "Cannot create calibration directory"; return false; }
    static std::atomic<unsigned> sequence{0};
    const std::string suffix = ".tmp." + std::to_string(::getpid()) + "." +
                               std::to_string(sequence.fetch_add(1));
    const std::filesystem::path temporary = file.string() + suffix;
    const int fd = ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) { error = "Cannot create temporary calibration file"; return false; }
    const bool written = write_all(fd, output) && ::fsync(fd) == 0;
    const bool closed = ::close(fd) == 0;
    if (!written || !closed) {
        std::filesystem::remove(temporary, ec);
        error = "Cannot write temporary calibration file";
        return false;
    }
    if (existing) {
        const std::filesystem::path backup = file.string() + ".bak";
        const std::filesystem::path backup_temp = backup.string() + suffix;
        std::filesystem::copy_file(file, backup_temp, ec);
        if (!ec) {
            const int backup_fd = ::open(backup_temp.c_str(), O_RDONLY);
            if (backup_fd < 0 || ::fsync(backup_fd) != 0) ec = std::make_error_code(std::errc::io_error);
            if (backup_fd >= 0) ::close(backup_fd);
        }
        if (!ec) std::filesystem::rename(backup_temp, backup, ec);
        if (ec) {
            std::filesystem::remove(temporary, ec);
            std::filesystem::remove(backup_temp, ec);
            error = "Cannot back up previous calibration";
            return false;
        }
    }
    std::filesystem::rename(temporary, file, ec);
    if (ec) {
        std::filesystem::remove(temporary, ec);
        error = "Cannot replace calibration atomically";
        return false;
    }
    const int dir_fd = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY);
    if (dir_fd >= 0) {
        ::fsync(dir_fd);
        ::close(dir_fd);
    }
    return true;
}

sensors::Vec3 calibrate_magnetometer(const Calibration& calibration,
                                    sensors::Vec3 raw_uT) {
    const sensors::Vec3 centered{raw_uT.x - calibration.offset_uT.x,
                                 raw_uT.y - calibration.offset_uT.y,
                                 raw_uT.z - calibration.offset_uT.z};
    return multiply(calibration.sensor_to_body,
                    multiply(calibration.soft_iron, centered));
}

} // namespace zerosky::orientation
