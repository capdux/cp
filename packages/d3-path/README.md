[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-path` ([https://github.com/d3/d3-path](https://github.com/d3/d3-path)) | 许可：`ISC`

# `@cap/d3-path`

零堆分配纯 CP Canvas2D 风格 SVG 路径（`d` 属性）序列化构造器，对标 npm **#400** `d3-path`（真实周下载 **1.34 亿/周**）。

## 核心 API

- `SvgPath.new()` / `.moveTo(x, y)` / `.lineTo(x, y)` / `.quadraticCurveTo(...)` / `.bezierCurveTo(...)` / `.rect(x, y, w, h)` / `.closePath()` / `.writeTo(out)` / `.reset()`
