/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

// toolkit::i18n: locale names to languages, and the English fallback.

#include "docs.h"
#include "i18n.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace toolkit::i18n;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) {
        ++failures;
        std::fprintf(stderr, "FAIL: %s\n", what);
    }
}

int main() {
    check(parse_locale("it_IT.UTF-8") == Lang::It, "it_IT.UTF-8 -> it");
    check(parse_locale("fr_CA") == Lang::Fr, "fr_CA -> fr");
    check(parse_locale("de") == Lang::De, "de -> de");
    check(parse_locale("es-ES") == Lang::Es, "es-ES -> es");
    check(parse_locale("en_US.UTF-8") == Lang::En, "en_US -> en");
    check(parse_locale("ja_JP.UTF-8") == Lang::Ja, "ja_JP -> ja");
    check(parse_locale("zh_CN.UTF-8") == Lang::Zh, "zh_CN -> zh");
    check(parse_locale("zh_TW.UTF-8") == Lang::Zh, "zh_TW -> zh (Simplified)");
    check(parse_locale("ko_KR.UTF-8") == Lang::En, "ko (no translation) -> en");
    check(needs_cjk_font(Lang::Ja) && needs_cjk_font(Lang::Zh), "ja and zh need a CJK font");
    check(!needs_cjk_font(Lang::It), "it uses Inter");
    check(parse_locale("C") == Lang::En, "C -> en");
    check(parse_locale("") == Lang::En, "empty -> en");
    check(parse_locale(nullptr) == Lang::En, "null -> en");
    check(parse_locale("item") == Lang::En, "a word starting with a code is not that code");

    const Text full{"Zoom in", "Ingrandisci", "Zoom avant", "Vergrößern", "Acercar",
                    "ズームイン", "放大"};
    check(std::strcmp(pick(full, Lang::It), "Ingrandisci") == 0, "pick it");
    check(std::strcmp(pick(full, Lang::De), "Vergrößern") == 0, "pick de");
    check(std::strcmp(pick(full, Lang::En), "Zoom in") == 0, "pick en");
    check(std::strcmp(pick(full, Lang::Ja), "ズームイン") == 0, "pick ja");
    check(std::strcmp(pick(full, Lang::Zh), "放大") == 0, "pick zh");

    const Text partial{"Zoom in", nullptr, "", nullptr, nullptr, nullptr, ""};
    check(std::strcmp(pick(partial, Lang::It), "Zoom in") == 0, "null falls back to en");
    check(std::strcmp(pick(partial, Lang::Fr), "Zoom in") == 0, "empty falls back to en");
    check(std::strcmp(pick(partial, Lang::Ja), "Zoom in") == 0, "null ja falls back to en");
    check(std::strcmp(pick(partial, Lang::Zh), "Zoom in") == 0, "empty zh falls back to en");

    const Text none{};
    check(none.empty(), "default Text is empty");
    check(std::strcmp(pick(none, Lang::Es), "") == 0, "empty Text picks an empty string");

    // zerosky: the in-app language choice and the translated docs.
    check(std::strcmp(code(Lang::En), "en") == 0 && std::strcmp(code(Lang::Zh), "zh") == 0, "codes");
    const auto dir = std::filesystem::temp_directory_path() /
                     ("zerosky_i18n_test_" + std::to_string(getpid()));
    const auto file = dir / "zeroradio" / "lang";
    check(!load_choice(file), "no file -> follow the locale");
    check(save_choice(file, Lang::It) && load_choice(file) == Lang::It, "save and load it");
    check(save_choice(file, Lang::En) && load_choice(file) == Lang::En, "en is a real choice");
    check(save_choice(file, std::nullopt) && !std::filesystem::exists(file), "auto removes the file");
    check(save_choice(file, std::nullopt), "auto without a file is fine");
    std::filesystem::create_directories(file.parent_path());
    std::ofstream(file) << "de\r\n";
    check(load_choice(file) == Lang::De, "CRLF is tolerated");
    std::ofstream(file) << "auto\n";
    check(!load_choice(file), "auto -> follow the locale");
    std::ofstream(file) << "ko\n";
    check(!load_choice(file), "an unsupported code -> follow the locale");
    std::filesystem::remove_all(dir);

    using toolkit::localized_doc_path;
    check(localized_doc_path("docs/help/zerosky.md", "it") == "docs/help/zerosky.it.md", "help it");
    check(localized_doc_path("docs/help/zerosky.md", "en") == "docs/help/zerosky.md", "help en");
    check(localized_doc_path("https://x.org/a.b/docs/manual.md", "ja") ==
              "https://x.org/a.b/docs/manual.ja.md", "manual URL ja");
    check(localized_doc_path("docs.v2/README", "fr") == "docs.v2/README.fr", "no extension");

    if (failures == 0) std::printf("i18n_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
