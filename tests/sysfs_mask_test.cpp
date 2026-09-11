/*
 * Copyright 2026 FloralDroid
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "sysfs_mask.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

bool Check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool CreateFile(const fs::path &path) {
  std::ofstream stream(path);
  stream << "25000\n";
  return stream.good();
}

bool CreateDirectorySymlink(const fs::path &target, const fs::path &link) {
  std::error_code error;
  fs::create_directory_symlink(target, link, error);
  return !error;
}

} // namespace

int main() {
  char path_template[] = "/tmp/floral-sysfs-mask-XXXXXX";
  const char *temporary = mkdtemp(path_template);
  if (!Check(temporary != nullptr, "create temporary directory")) {
    return EXIT_FAILURE;
  }

  const fs::path root(temporary);
  const fs::path devices = root / "devices";
  const fs::path class_hwmon = root / "class/hwmon";
  const fs::path cpu_hwmon = devices / "platform/cpu-provider/hwmon";
  const fs::path gpu_hwmon = devices / "pci/gpu-provider/hwmon";
  const fs::path fan_hwmon = devices / "platform/fan/hwmon";
  const fs::path invalid_hwmon = devices / "platform/invalid/hwmon";
  const fs::path outside_hwmon = root / "outside/hwmon";

  std::error_code error;
  fs::create_directories(cpu_hwmon / "hwmon1", error);
  fs::create_directories(gpu_hwmon / "hwmon2", error);
  fs::create_directories(fan_hwmon / "hwmon3", error);
  fs::create_directories(invalid_hwmon / "hwmon4", error);
  fs::create_directories(outside_hwmon / "hwmon5", error);
  fs::create_directories(class_hwmon, error);

  bool ok = Check(!error, "create test tree") &&
            Check(CreateFile(cpu_hwmon / "hwmon1/temp1_input"),
                  "create CPU temperature input") &&
            Check(CreateFile(gpu_hwmon / "hwmon2/temp12_input"),
                  "create GPU temperature input") &&
            Check(CreateFile(fan_hwmon / "hwmon3/fan1_input"),
                  "create non-temperature input") &&
            Check(CreateFile(invalid_hwmon / "hwmon4/temp_input"),
                  "create invalid temperature input") &&
            Check(CreateFile(outside_hwmon / "hwmon5/temp1_input"),
                  "create outside temperature input");

  ok = Check(
           CreateDirectorySymlink(cpu_hwmon / "hwmon1", class_hwmon / "hwmon0"),
           "link CPU provider") &&
       Check(
           CreateDirectorySymlink(gpu_hwmon / "hwmon2", class_hwmon / "hwmon1"),
           "link GPU provider") &&
       Check(
           CreateDirectorySymlink(fan_hwmon / "hwmon3", class_hwmon / "hwmon2"),
           "link non-temperature provider") &&
       Check(CreateDirectorySymlink(invalid_hwmon / "hwmon4",
                                    class_hwmon / "hwmon3"),
             "link invalid temperature provider") &&
       Check(CreateDirectorySymlink(outside_hwmon / "hwmon5",
                                    class_hwmon / "hwmon4"),
             "link provider outside devices") &&
       Check(
           CreateDirectorySymlink(gpu_hwmon / "hwmon2", class_hwmon / "hwmon5"),
           "link duplicate GPU provider") &&
       ok;

  std::vector<std::string> directories;
  ok = Check(floral::lxcfs::FindTemperatureHwmonDirectories(
                 class_hwmon.string(), devices.string(), &directories),
             "scan sysfs tree") &&
       Check(directories == std::vector<std::string>(
                                {gpu_hwmon.string(), cpu_hwmon.string()}),
             "find only real temperature hwmon directories") &&
       Check(!floral::lxcfs::FindTemperatureHwmonDirectories(
                 (root / "missing").string(), devices.string(), &directories),
             "reject missing hwmon class") &&
       ok;

  fs::remove_all(root, error);
  ok = Check(!error, "remove test tree") && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
