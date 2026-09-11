/*
 * Copyright 2026 FloralDroid
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#pragma once

#include <string>
#include <vector>

namespace floral::lxcfs {

bool FindTemperatureHwmonDirectories(const std::string &class_root,
                                     const std::string &devices_root,
                                     std::vector<std::string> *directories);

} // namespace floral::lxcfs
