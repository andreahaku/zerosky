/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <filesystem>
#include <optional>

namespace toolkit::i18n {

// UI languages with translations. Chinese is Simplified for every zh locale.
enum class Lang { En, It, Fr, De, Es, Ja, Zh };

// One string in every supported language. A null entry falls back to `en`.
struct Text {
    const char* en{nullptr};
    const char* it{nullptr};
    const char* fr{nullptr};
    const char* de{nullptr};
    const char* es{nullptr};
    const char* ja{nullptr};
    const char* zh{nullptr};

    bool empty() const { return en == nullptr || en[0] == '\0'; }
};

// Language from a POSIX locale name ("it_IT.UTF-8", "de", "C"): English when
// the name is empty or not supported.
Lang parse_locale(const char* name);

// True for the languages whose glyphs the bundled Inter font lacks.
bool needs_cjk_font(Lang lang);

// The user's language: ZERORADIO_LANG, then the language chosen in an app
// (choice_file()), then LC_ALL, LC_MESSAGES and LANG, the way the
// CardputerZero Store reads the locale. Read once, then cached: a new choice
// applies at the next start.
// zerosky: the saved choice is a ZeroSky addition.
Lang current();

// zerosky: the language picked inside an app, shared by the suite: one line
// with a two-letter code in ~/.config/zeroradio/lang. No file (or "auto")
// means "follow the locale".
std::filesystem::path choice_file();
std::optional<Lang> load_choice(const std::filesystem::path& file);
// Writes the code, or removes the file for nullopt. False on a write error.
bool save_choice(const std::filesystem::path& file, std::optional<Lang> lang);

// Two-letter code ("en", "it", ...).
const char* code(Lang lang);

// `text` in `lang` (or in current()), falling back to English.
const char* pick(const Text& text, Lang lang);
const char* pick(const Text& text);

} // namespace toolkit::i18n
