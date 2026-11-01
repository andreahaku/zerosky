/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "docs.h"

#include "i18n.h"

#include <fstream>
#include <sstream>

#ifndef ZERORADIO_DOCS_ROOT
#define ZERORADIO_DOCS_ROOT "/usr/share/zeroradio"
#endif

namespace toolkit {

std::string read_doc(const std::string& relative_path) {
    std::ifstream in(std::string(ZERORADIO_DOCS_ROOT) + "/" + relative_path);
    if (!in) return {};
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

std::string localized_doc_path(const std::string& relative_path, const char* lang_code) {
    if (!lang_code || std::string(lang_code) == "en") return relative_path;
    const auto slash = relative_path.find_last_of('/');
    const auto dot = relative_path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        return relative_path + "." + lang_code;
    }
    return relative_path.substr(0, dot) + "." + lang_code + relative_path.substr(dot);
}

std::string read_localized_doc(const std::string& relative_path) {
    const std::string translated =
        read_doc(localized_doc_path(relative_path, i18n::code(i18n::current())));
    return translated.empty() ? read_doc(relative_path) : translated;
}

} // namespace toolkit
