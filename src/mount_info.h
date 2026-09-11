/*
 * Copyright 2026 FloralDroid
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#pragma once

#include <string>
#include <string_view>

namespace floral::lxcfs {

std::string UnescapeMountInfoPath(std::string_view value);

bool MountInfoHasFilesystem(std::string_view mountinfo,
                            std::string_view mountpoint,
                            std::string_view filesystem);

bool MountInfoHasMountpoint(std::string_view mountinfo,
                            std::string_view mountpoint);

}  // namespace floral::lxcfs
