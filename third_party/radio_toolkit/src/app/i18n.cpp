/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "i18n.h"

#include "persisted_state.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <string>

namespace toolkit::i18n {

Lang parse_locale(const char* name) {
    if (!name || std::strlen(name) < 2) return Lang::En;
    const auto is = [name](const char* code) {
        return std::strncmp(name, code, 2) == 0 && (name[2] == '\0' || name[2] == '_' ||
                                                    name[2] == '-' || name[2] == '.');
    };
    if (is("it")) return Lang::It;
    if (is("fr")) return Lang::Fr;
    if (is("de")) return Lang::De;
    if (is("es")) return Lang::Es;
    if (is("ja")) return Lang::Ja;
    if (is("zh")) return Lang::Zh;
    return Lang::En;
}

const char* code(Lang lang) {
    switch (lang) {
        case Lang::It: return "it";
        case Lang::Fr: return "fr";
        case Lang::De: return "de";
        case Lang::Es: return "es";
        case Lang::Ja: return "ja";
        case Lang::Zh: return "zh";
        case Lang::En: break;
    }
    return "en";
}

// zerosky: the in-app language choice.
std::filesystem::path choice_file() {
    return config_file("zeroradio", "lang");
}

std::optional<Lang> load_choice(const std::filesystem::path& file) {
    if (file.empty()) return std::nullopt;
    std::ifstream in(file);
    std::string line;
    if (!in || !std::getline(in, line)) return std::nullopt;
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
    // "en" is a real choice; anything else unknown means "follow the locale".
    if (line == "en") return Lang::En;
    const Lang lang = parse_locale(line.c_str());
    if (lang == Lang::En) return std::nullopt;
    return lang;
}

bool save_choice(const std::filesystem::path& file, std::optional<Lang> lang) {
    if (file.empty()) return false;
    std::error_code ec;
    if (!lang) {
        std::filesystem::remove(file, ec);
        return !ec;
    }
    if (!ensure_parent_dir(file)) return false;
    // A two-letter file: a temporary copy and a rename keep it whole.
    std::filesystem::path temp = file;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        out << code(*lang) << '\n';
        if (!out.flush()) return false;
    }
    std::filesystem::rename(temp, file, ec);
    if (ec) {
        std::error_code ignored;
        std::filesystem::remove(temp, ignored);
        return false;
    }
    return true;
}

Lang current() {
    static const Lang lang = [] {
        if (const char* value = std::getenv("ZERORADIO_LANG"); value && value[0] != '\0') {
            return parse_locale(value);
        }
        if (const auto chosen = load_choice(choice_file())) return *chosen;
        // The first variable that is set wins, as in the C library: LC_ALL
        // overrides LANG even when it names a language without translations.
        for (const char* var : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
            const char* value = std::getenv(var);
            if (value && value[0] != '\0') return parse_locale(value);
        }
        return Lang::En;
    }();
    return lang;
}

const char* pick(const Text& text, Lang lang) {
    const char* value = nullptr;
    switch (lang) {
        case Lang::It: value = text.it; break;
        case Lang::Fr: value = text.fr; break;
        case Lang::De: value = text.de; break;
        case Lang::Es: value = text.es; break;
        case Lang::Ja: value = text.ja; break;
        case Lang::Zh: value = text.zh; break;
        case Lang::En: break;
    }
    if (value && value[0] != '\0') return value;
    return text.en ? text.en : "";
}

bool needs_cjk_font(Lang lang) {
    return lang == Lang::Ja || lang == Lang::Zh;
}

const char* pick(const Text& text) {
    return pick(text, current());
}

} // namespace toolkit::i18n
