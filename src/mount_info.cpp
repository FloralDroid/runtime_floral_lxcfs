/*
 * Copyright 2026 FloralDroid
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "mount_info.h"

#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace floral::lxcfs {
namespace {

std::vector<std::string> SplitWhitespace(std::string_view value) {
    std::istringstream stream{std::string(value)};
    std::vector<std::string> fields;
    std::string field;
    while (stream >> field) {
        fields.push_back(std::move(field));
    }
    return fields;
}

std::string UnescapeField(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (size_t index = 0; index < value.size(); ++index) {
        if (value[index] != '\\' || index + 3 >= value.size()) {
            result.push_back(value[index]);
            continue;
        }

        const std::string_view escape = value.substr(index, 4);
        if (escape == "\\040") {
            result.push_back(' ');
            index += 3;
        } else if (escape == "\\011") {
            result.push_back('\t');
            index += 3;
        } else if (escape == "\\012") {
            result.push_back('\n');
            index += 3;
        } else if (escape == "\\134") {
            result.push_back('\\');
            index += 3;
        } else {
            result.push_back(value[index]);
        }
    }
    return result;
}

}  // namespace

std::string UnescapeMountInfoPath(std::string_view value) {
    return UnescapeField(value);
}

static bool MountInfoMatches(std::string_view mountinfo,
                             std::string_view mountpoint,
                             std::string_view filesystem) {
    size_t line_start = 0;
    while (line_start < mountinfo.size()) {
        const size_t line_end = mountinfo.find('\n', line_start);
        const size_t line_size = line_end == std::string_view::npos
                ? mountinfo.size() - line_start
                : line_end - line_start;
        const std::string_view line = mountinfo.substr(line_start, line_size);
        const size_t separator = line.find(" - ");
        if (separator != std::string_view::npos) {
            const std::vector<std::string> left =
                    SplitWhitespace(line.substr(0, separator));
            const std::vector<std::string> right =
                    SplitWhitespace(line.substr(separator + 3));
            // mountinfo fields are: id, parent, major:minor, root,
            // mount point, mount options, optional fields...
            // The post-separator first field is the filesystem type.
            if (left.size() >= 5 && !right.empty() &&
                UnescapeField(left[4]) == mountpoint &&
                (filesystem.empty() || right[0] == filesystem)) {
                return true;
            }
        }
        if (line_end == std::string_view::npos) {
            break;
        }
        line_start = line_end + 1;
    }
    return false;
}

bool MountInfoHasFilesystem(std::string_view mountinfo,
                            std::string_view mountpoint,
                            std::string_view filesystem) {
    return MountInfoMatches(mountinfo, mountpoint, filesystem);
}

bool MountInfoHasMountpoint(std::string_view mountinfo,
                            std::string_view mountpoint) {
    return MountInfoMatches(mountinfo, mountpoint, {});
}

}  // namespace floral::lxcfs
