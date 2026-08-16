/*
 * Copyright 2026 FloralDroid
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "mount_info.h"

#include <android-base/logging.h>

#include <cerrno>
#include <sys/mount.h>
#include <sys/stat.h>

#include <fstream>
#include <iterator>
#include <string>

namespace {

constexpr char kSourceRoot[] = "/run/floral-lxcfs";
constexpr char kMountInfo[] = "/proc/self/mountinfo";
constexpr char kFloralFilesystem[] = "fuse.floral-lxcfs";

struct View {
  const char *source;
  const char *target;
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
  if (!Exists(view.source) || !Exists(view.target)) {
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
  for (const View &view : kViews) {
    success = BindView(view, mountinfo) && success;
  }
  return success ? 0 : 1;
}
