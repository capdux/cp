[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ieee754` ([https://github.com/feross/ieee754](https://github.com/feross/ieee754)) | 许可：`BSD-3-Clause`

# `@cap/ieee754`

零堆分配纯 CP IEEE-754 单精度（f32）与双精度（f64）大小端二进制读写引擎，对标 npm **#293** `ieee754`（真实周下载 **1.73 亿/周**，Node.js `Buffer` 底层核心依赖），位级保真处理 `-0.0`、`NaN`、`±Infinity` 与次正规数。

## 核心 API

- `readF32Le` / `readF32Be` / `writeF32Le` / `writeF32Be`
- `readF64Le` / `readF64Be` / `writeF64Le` / `writeF64Be`
- `readIeee754` / `writeIeee754`
