# Floral Android LXCFS helper

This project contains the Android-side integration for the host
`floral-lxcfs` FUSE service. It is intentionally separate from the host FUSE
implementation and is checked out at `system/floral/lxcfs` in the Android
source tree.

The `floral_lxcfs_bind` binary is a short-lived init helper. It does nothing
when `/run/floral-lxcfs` is absent, and otherwise binds the available CPU,
memory, proc and sysfs views after Android has mounted its final procfs.

The container launcher should expose the host mount with:

```text
--mount type=bind,src=/var/lib/floral-lxcfs,dst=/run/floral-lxcfs,readonly
```

The helper does not replace `/proc` or `/sys` and does not affect containers
that do not enable the host LXCFS mount.
