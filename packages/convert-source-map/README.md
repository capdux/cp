[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:convert-source-map` ([https://github.com/thlorenz/convert-source-map](https://github.com/thlorenz/convert-source-map)) | 许可：`MIT`

# `convert-source-map`

零堆分配纯 CP 内联 SourceMap 注释解析、Base64 编解码与注释剥离引擎，对标 npm **#96** `convert-source-map`（真实周下载 **3.07 亿/周**）。

## 核心 API

- `findSourceMapComment(src: string): SourceMapComment` — 识别 `//#` / `/*#` 内联 Base64 或外链 `sourceMappingURL`
- `fromSource(src: string, out: []u8): i64` — 从源码提取并解码内联 Base64 SourceMap JSON
- `toComment(json_map: string, multiline: bool, out: []u8): usize` — 生成单行 JS 或多行 CSS 内联 SourceMap 注释
- `removeComments(src: string, out: []u8): usize` — 从源码中剥离 `sourceMappingURL` 注释
