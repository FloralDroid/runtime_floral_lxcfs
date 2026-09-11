/*
 * Copyright 2026 FloralDroid
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "mount_info.h"
#include "sysfs_mask.h"

#include <android-base/logging.h>
#include <android-base/properties.h>

#include <cerrno>
#include <cstdint>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr char kSourceRoot[] = "/run/floral-lxcfs";
constexpr char kMountInfo[] = "/proc/self/mountinfo";
constexpr char kFloralFilesystem[] = "fuse.floral-lxcfs";
constexpr char kSysDevices[] = "/sys/devices";
constexpr char kSysClassHwmon[] = "/sys/class/hwmon";
constexpr char kEmptyHwmonSource[] = "/run/floral-lxcfs/sys/class/hwmon";
constexpr char kFramebufferModes[] = "/sys/class/graphics/fb0/modes";
constexpr char kFramebufferVirtualSize[] =
    "/sys/class/graphics/fb0/virtual_size";

struct View {
  const char *source;
  const char *target;
};

struct GeneratedView {
  const char *target;
  std::string content;
};

constexpr View kViews[] = {
    {"/run/floral-lxcfs/proc/cpuinfo", "/proc/cpuinfo"},
    {"/run/floral-lxcfs/proc/meminfo", "/proc/meminfo"},
    {"/run/floral-lxcfs/proc/stat", "/proc/stat"},
    {"/run/floral-lxcfs/proc/swaps", "/proc/swaps"},
    {"/run/floral-lxcfs/proc/uptime", "/proc/uptime"},
    {"/run/floral-lxcfs/proc/loadavg", "/proc/loadavg"},
    {"/run/floral-lxcfs/proc/slabinfo", "/proc/slabinfo"},
    {"/run/floral-lxcfs/proc/zoneinfo", "/proc/zoneinfo"},
    {"/run/floral-lxcfs/proc/vmstat", "/proc/vmstat"},
    {"/run/floral-lxcfs/proc/buddyinfo", "/proc/buddyinfo"},
    {"/run/floral-lxcfs/proc/version", "/proc/version"},
    {"/run/floral-lxcfs/proc/sys/kernel/osrelease",
     "/proc/sys/kernel/osrelease"},
    {"/run/floral-lxcfs/proc/diskstats", "/proc/diskstats"},
    {"/run/floral-lxcfs/proc/pressure/cpu", "/proc/pressure/cpu"},
    {"/run/floral-lxcfs/proc/pressure/io", "/proc/pressure/io"},
    {"/run/floral-lxcfs/proc/pressure/memory", "/proc/pressure/memory"},
    {"/run/floral-lxcfs/sys/devices/system/cpu", "/sys/devices/system/cpu"},
    {"/run/floral-lxcfs/sys/devices/system/node", "/sys/devices/system/node"},
    {"/run/floral-lxcfs/sys/block", "/sys/block"},
    {"/run/floral-lxcfs/sys/devices/virtual/dmi/id",
     "/sys/devices/virtual/dmi/id"},
    {"/run/floral-lxcfs/sys/devices/virtual/thermal",
     "/sys/devices/virtual/thermal"},
    {"/run/floral-lxcfs/sys/class/thermal", "/sys/class/thermal"},
    {"/run/floral-lxcfs/sys/class/hwmon", "/sys/class/hwmon"},
};

bool Exists(const char *path) {
  struct stat status = {};
  return stat(path, &status) == 0;
}

bool IsDirectory(const char *path) {
  struct stat status = {};
  return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
}

std::string ReadMountInfo() {
  std::ifstream stream(kMountInfo);
  if (!stream.is_open()) {
    return {};
  }
  return {std::istreambuf_iterator<char>(stream),
          std::istreambuf_iterator<char>()};
}

bool IsAlreadyMounted(const std::string &mountinfo, const char *target) {
  return floral::lxcfs::MountInfoHasFilesystem(mountinfo, target,
                                               kFloralFilesystem);
}

bool BindView(const View &view, const std::string &mountinfo) {
  if (!Exists(view.source)) {
    return true;
  }
  if (!Exists(view.target)) {
    LOG(WARNING) << "LXCFS target is unavailable; skipping " << view.target;
    return true;
  }

  if (IsAlreadyMounted(mountinfo, view.target)) {
    LOG(INFO) << "LXCFS view already mounted at " << view.target;
    return true;
  }

  if (mount(view.source, view.target, nullptr, MS_BIND, nullptr) == 0) {
    LOG(INFO) << "Mounted LXCFS view " << view.target;
    return true;
  }

  if (errno == ENOENT || errno == ENOTDIR) {
    // The optional source or target disappeared during startup.
    return true;
  }
  PLOG(ERROR) << "Unable to bind LXCFS view " << view.source << " to "
              << view.target;
  return false;
}

bool WriteAll(int fd, std::string_view content) {
  size_t offset = 0;
  while (offset < content.size()) {
    const ssize_t written =
        write(fd, content.data() + offset, content.size() - offset);
    if (written < 0 && errno == EINTR) {
      continue;
    }
    if (written <= 0) {
      return false;
    }
    offset += static_cast<size_t>(written);
  }
  return true;
}

bool BindGeneratedView(const GeneratedView &view,
                       const std::string &mountinfo) {
  if (!Exists(view.target)) {
    LOG(WARNING) << "Generated view target is unavailable; skipping "
                 << view.target;
    return true;
  }
  if (floral::lxcfs::MountInfoHasMountpoint(mountinfo, view.target)) {
    LOG(INFO) << "Generated view already mounted at " << view.target;
    return true;
  }

  const int fd = memfd_create("floral-display-view", MFD_CLOEXEC);
  if (fd < 0) {
    PLOG(ERROR) << "Unable to create generated view for " << view.target;
    return false;
  }
  if (!WriteAll(fd, view.content) || fchmod(fd, 0444) != 0) {
    PLOG(ERROR) << "Unable to populate generated view for " << view.target;
    close(fd);
    return false;
  }

  const std::string source = "/proc/self/fd/" + std::to_string(fd);
  if (mount(source.c_str(), view.target, nullptr, MS_BIND, nullptr) != 0) {
    PLOG(ERROR) << "Unable to bind generated view to " << view.target;
    close(fd);
    return false;
  }
  close(fd);
  LOG(INFO) << "Mounted generated view " << view.target;
  return true;
}

uint32_t BoundedProperty(const char *name, uint32_t default_value,
                         uint32_t minimum, uint32_t maximum) {
  const uint32_t value = android::base::GetUintProperty<uint32_t>(
      name, default_value, maximum);
  return value < minimum ? default_value : value;
}

std::vector<GeneratedView> DisplayViews() {
  const uint32_t width =
      BoundedProperty("ro.boot.floral_width", 1920, 320, 7680);
  const uint32_t height =
      BoundedProperty("ro.boot.floral_height", 1080, 320, 4320);
  const uint32_t fps = BoundedProperty("ro.boot.floral_fps", 60, 1, 60);
  return {
      {kFramebufferVirtualSize,
       std::to_string(width) + "," + std::to_string(height) + "\n"},
      {kFramebufferModes, "U:" + std::to_string(width) + "x" +
                              std::to_string(height) + "p-" +
                              std::to_string(fps) + "\n"},
  };
}

} // namespace

int main(int /* argc */, char **argv) {
  android::base::InitLogging(argv, &android::base::KernelLogger);

  if (!IsDirectory(kSourceRoot)) {
    LOG(INFO) << "LXCFS source is not mounted; skipping optional views";
    return 0;
  }

  const std::string mountinfo = ReadMountInfo();
  if (mountinfo.empty()) {
    LOG(ERROR) << "Unable to read " << kMountInfo;
    return 1;
  }

  bool success = true;
  std::vector<std::string> temperature_hwmon_directories;
  if (IsDirectory(kSysClassHwmon) &&
      !floral::lxcfs::FindTemperatureHwmonDirectories(
          kSysClassHwmon, kSysDevices, &temperature_hwmon_directories)) {
    LOG(ERROR) << "Unable to inspect " << kSysClassHwmon
               << " for temperature providers";
    success = false;
  }

  for (const View &view : kViews) {
    success = BindView(view, mountinfo) && success;
  }
  for (const std::string &target : temperature_hwmon_directories) {
    const View view = {kEmptyHwmonSource, target.c_str()};
    success = BindView(view, mountinfo) && success;
  }
  for (const GeneratedView &view : DisplayViews()) {
    success = BindGeneratedView(view, mountinfo) && success;
  }
  return success ? 0 : 1;
}
