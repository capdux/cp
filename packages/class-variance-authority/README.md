[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/class-variance-authority

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:class-variance-authority` ([https://github.com/joe-bell/cva](https://github.com/joe-bell/cva))（原库名 `class-variance-authority` → CP 包名 `@cap/class-variance-authority`）

零堆分配的 CP 原生 CSS 变体样式组合器，Clean-Room Apache-2.0 实现对标前端组件库顶流包 `class-variance-authority` / `cva`（85.0M+/周下载）。

支持基础样式、多维度变体规则、默认变体覆盖、多条件复合变体叠加、用户额外类名追加以及自动空白折叠规整，全程零堆分配写入调用方缓冲。

## 核心 API

- `VariantRule` — 单个变体规则（`group`, `option`, `classes`）
- `CompoundRule` — 复合变体规则（支持最多 3 组 `(group, option)` 条件联合匹配，提供 `CompoundRule.pair(...)` / `CompoundRule.triple(...)`）
- `VariantSelection` — 变体选项（`group`, `option`）
- `cvaCompose(base, rules, defaults, selected, extra, out): usize` — 组合基础类名、变体规则（`selected` 覆盖 `defaults`）与额外类名
- `cvaComposeWithCompounds(base, rules, compounds, defaults, selected, extra, out): usize` — 在 `cvaCompose` 基础上叠加复合变体规则

## 致谢

Clean-Room Apache-2.0 实现，API 语义对标开源生态 [`joe-bell/cva`](https://github.com/joe-bell/cva)。
