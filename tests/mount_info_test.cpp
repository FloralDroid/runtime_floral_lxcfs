/*
 * Copyright 2026 FloralDroid
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "mount_info.h"

#include <cstdlib>
#include <iostream>

namespace {

bool Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

}  // namespace

int main() {
    const std::string mountinfo =
            "41 31 0:42 / /proc/cpuinfo rw - fuse.floral-lxcfs "
            "/run/floral-lxcfs/proc/cpuinfo rw\n"
            "42 31 0:43 / /proc/meminfo rw - proc proc rw\n"
            "43 31 0:44 /memfd:floral /sys/class/graphics/fb0/virtual_size ro - "
            "tmpfs memfd:floral rw\n";

    const bool ok =
            Check(floral::lxcfs::UnescapeMountInfoPath("/run/a\\040b") ==
                          "/run/a b",
                  "mountinfo space escape") &&
            Check(floral::lxcfs::MountInfoHasFilesystem(
                          mountinfo, "/proc/cpuinfo", "fuse.floral-lxcfs"),
                  "find Floral LXCFS mount") &&
            Check(!floral::lxcfs::MountInfoHasFilesystem(
                          mountinfo, "/proc/meminfo", "fuse.floral-lxcfs"),
                  "reject unrelated filesystem") &&
            Check(!floral::lxcfs::MountInfoHasFilesystem(
                          mountinfo, "/proc/stat", "fuse.floral-lxcfs"),
                  "reject unrelated mountpoint") &&
            Check(floral::lxcfs::MountInfoHasMountpoint(
                          mountinfo, "/sys/class/graphics/fb0/virtual_size"),
                  "find generated file bind mount") &&
            Check(!floral::lxcfs::MountInfoHasMountpoint(mountinfo, "/proc/stat"),
                  "reject absent mountpoint");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
