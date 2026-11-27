[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pathe` ([https://github.com/unjs/pathe](https://github.com/unjs/pathe)) | 许可：`MIT`

# `pathe`

零堆分配纯 CP 跨平台路径处理库（始终输出 POSIX `/` 分隔符），完整对标 npm **#219** `pathe`（真实周下载 **2.07 亿/周**，UnJS / Nuxt / Nitro / Vite 生态核心依赖）。

## 核心 API

- `normalize(path: string, out: []u8): usize`：将 `\` 转为 `/`，保留 Windows 盘符（`C:/`）与协议前缀（`file://`、`https://`），折叠 `//` 并解析 `.` / `..`
- `join(a: string, b: string, out: []u8): usize`、`resolve(base: string, rel: string, out: []u8): usize`
- `dirname(path: string): string`、`basename(path: string): string`、`extname(path: string): string`
- `isAbsolute(path: string): bool`、`relative(from_path: string, to_path: string, out: []u8): usize`
