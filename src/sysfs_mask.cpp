/*
 * Copyright 2026 FloralDroid
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "sysfs_mask.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <filesystem>
#include <string_view>
#include <system_error>

namespace floral::lxcfs {
namespace {

namespace fs = std::filesystem;

bool IsTemperatureInputName(std::string_view name) {
  if (name.compare(0, 4, "temp") != 0) {
    return false;
  }

  size_t index = 4;
  while (index < name.size() &&
         std::isdigit(static_cast<unsigned char>(name[index]))) {
    ++index;
  }
  return index > 4 && name.substr(index) == "_input";
}

bool IsWithin(const fs::path &path, const fs::path &root) {
  auto path_component = path.begin();
  for (const fs::path &root_component : root) {
    if (path_component == path.end() || *path_component != root_component) {
      return false;
    }
    ++path_component;
  }
  return true;
}

bool ContainsTemperatureInput(const fs::path &provider, bool *complete) {
  std::error_code error;
  fs::directory_iterator attributes(provider, error);
  if (error) {
    *complete = false;
    return false;
  }

  const fs::directory_iterator end;
  while (attributes != end) {
    const fs::directory_entry attribute = *attributes;
    attributes.increment(error);
    if (error) {
      *complete = false;
      error.clear();
    }
    if (IsTemperatureInputName(attribute.path().filename().string())) {
      return true;
    }
  }
  return false;
}

} // namespace

bool FindTemperatureHwmonDirectories(const std::string &class_root,
                                     const std::string &devices_root,
                                     std::vector<std::string> *directories) {
  if (!directories) {
    return false;
  }
  directories->clear();

  std::error_code error;
  const fs::path canonical_devices = fs::canonical(devices_root, error);
  if (error) {
    return false;
  }

  bool complete = true;
  fs::directory_iterator entries(class_root, error);
  if (error) {
    return false;
  }

  const fs::directory_iterator end;
  while (entries != end) {
    const fs::directory_entry entry = *entries;
    entries.increment(error);
    if (error) {
      complete = false;
      error.clear();
    }

    const fs::file_status status = entry.symlink_status(error);
    if (error) {
      if (error.value() != ENOENT) {
        complete = false;
      }
      error.clear();
      continue;
    }
    if (!fs::is_symlink(status)) {
      continue;
    }

    const fs::path provider = fs::canonical(entry.path(), error);
    if (error) {
      if (error.value() != ENOENT) {
        complete = false;
      }
      error.clear();
      continue;
    }

    const fs::path hwmon = provider.parent_path();
    if (!IsWithin(provider, canonical_devices) || hwmon.filename() != "hwmon" ||
        !ContainsTemperatureInput(provider, &complete)) {
      continue;
    }
    directories->push_back(hwmon.string());
  }

  std::sort(directories->begin(), directories->end());
  directories->erase(std::unique(directories->begin(), directories->end()),
                     directories->end());
  return complete;
}

} // namespace floral::lxcfs
