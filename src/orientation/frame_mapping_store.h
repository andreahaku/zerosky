/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "attitude_service.h"

#include <filesystem>
#include <string>

namespace zerosky::orientation {

// $XDG_CONFIG_HOME/zerosky/frame-mapping.json or ~/.config/zerosky/...
std::filesystem::path default_frame_mapping_file();

// A failed load resets result to an unverified mapping. Only a complete,
// validated schema-1 file can set result.verified to true.
bool load_frame_mapping(const std::filesystem::path& file,
                        FrameMapping& result, std::string& error);

// Requires mapping.verified. Writes a sibling temporary file, fsyncs it,
// backs up the previous complete file at .bak, then renames atomically.
bool save_frame_mapping(const std::filesystem::path& file,
                        const FrameMapping& mapping, std::string& error);

} // namespace zerosky::orientation
