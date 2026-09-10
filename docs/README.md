# Floral Android LXCFS helper

[简体中文](README.zh-CN.md)

This project contains the Android-side integration for the host
`floral-lxcfs` FUSE service. It is intentionally separate from the host FUSE
implementation and is checked out at `system/floral/lxcfs` in the Android
source tree.

The `floral_lxcfs_bind` binary is a short-lived init helper. It does nothing
when `/run/floral-lxcfs` is absent, and otherwise binds the available CPU,
memory, proc and sysfs views after Android has mounted its final procfs. This
includes the cgroup-backed `zoneinfo`, `vmstat`, `buddyinfo`, profile-backed
kernel identity, CPU topology and single-node NUMA views exposed by Floral
LXCFS. When the canonical `/sys/devices/virtual/dmi/id` target exists, the
helper also masks it with the profile-backed DMI identity view. The usual
`/sys/class/dmi/id` symlink then resolves to the same mounted directory.
The helper also binds the profile-backed battery thermal zone at
`/sys/devices/virtual/thermal` and `/sys/class/thermal`, and masks
`/sys/class/hwmon` with an empty view.

The container launcher should expose the host mount with:

```text
--mount type=bind,src=/var/lib/floral-lxcfs,dst=/run/floral-lxcfs,readonly
```

The helper does not replace `/proc` or `/sys` and does not affect containers
that do not enable the host LXCFS mount.
