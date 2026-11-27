[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-fullwidth-code-point` ([https://github.com/sindresorhus/is-fullwidth-code-point](https://github.com/sindresorhus/is-fullwidth-code-point)) | 许可：`MIT`

# `@cap/is-fullwidth-code-point`

零堆分配纯 CP Unicode UAX #11 全角码点判定库，对标 npm Top-20 包 `is-fullwidth-code-point` v5（周下载 1.8 亿+）。

## 核心 API

- `isFullwidthCodePoint(cp: u32): bool` — 判断码点是否为东亚宽/全角字符（CJK/Hangul/全角标点/Emoji）
- `codePointWidth(cp: u32): usize` — 返回单码点显示列宽（0 / 1 / 2）
- `countFullwidthChars(s: string): usize` — 统计 UTF-8 字符串中的全角字符数量
