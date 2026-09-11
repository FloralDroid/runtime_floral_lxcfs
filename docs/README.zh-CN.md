# Floral Android LXCFS 辅助组件

[English](README.en.md)

本项目包含主机端 `floral-lxcfs` FUSE 服务的 Android 侧集成代码。
它与主机 FUSE 实现刻意分离，在 Android 源码树中位于 `system/floral/lxcfs`。

二进制程序 `floral_lxcfs_bind` 是一个短生命周期的 init 辅助进程。
当 `/run/floral-lxcfs` 不存在时，它不做任何操作；否则，它会在 Android 挂载最终的 procfs 之后，绑定可用的 CPU、内存、proc 和 sysfs 视图。
这些视图包括由 Floral LXCFS 提供的、基于 cgroup 的 `zoneinfo`、`vmstat`、`buddyinfo`，基于配置文件的核标识、CPU 拓扑以及单节点 NUMA 视图。
当规范的 `/sys/devices/virtual/dmi/id` 目标存在时，该辅助进程还会用配置文件提供的 DMI 标识视图将其屏蔽（mask），通常的 `/sys/class/dmi/id` 软链接会解析到同一个挂载目录。
辅助进程还会将配置文件提供的电池热区绑定到 `/sys/devices/virtual/thermal` 和
`/sys/class/thermal`，并用空视图屏蔽 `/sys/class/hwmon`。它还会在
`/sys/devices` 下查找包含 `tempN_input` 文件的真实、非软链接 `hwmon` 目录，
并只屏蔽这些目录。这样既能阻止应用通过 platform 或 PCI 路径直接读取宿主
温度，也不会隐藏设备树的其他部分。
辅助进程还会根据 `ro.boot.floral_width`、`ro.boot.floral_height` 和
`ro.boot.floral_fps` 生成 framebuffer 的 `virtual_size` 与 `modes` 视图，
使这两个节点呈现容器实际显示配置，而不是宿主显示模式。

容器启动器应通过以下挂载选项暴露主机挂载点：

```text
--mount type=bind,src=/var/lib/floral-lxcfs,dst=/run/floral-lxcfs,readonly
