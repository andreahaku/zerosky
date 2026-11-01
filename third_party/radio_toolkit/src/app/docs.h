/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

namespace toolkit {

// Text shipped with the suite (help pages, CHANGELOG.md, CREDITS.md), by its
// path relative to the repository root, e.g. "docs/help/sdr.md". The package
// installs these files under /usr/share/zeroradio with the same relative paths;
// a desktop build reads them from the source tree. Empty if missing.
std::string read_doc(const std::string& relative_path);

// zerosky: the translation of a doc for the current language, e.g.
// "docs/help/sky.it.md" for "docs/help/sky.md", else the doc itself.
std::string read_localized_doc(const std::string& relative_path);

// zerosky: "docs/help/sky.md" with the language code before the extension
// ("docs/help/sky.it.md"); unchanged for English.
std::string localized_doc_path(const std::string& relative_path, const char* lang_code);

} // namespace toolkit
