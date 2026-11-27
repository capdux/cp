[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# abbrev

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:abbrev` ([https://github.com/npm/abbrev-js](https://github.com/npm/abbrev-js))（原库名 `abbrev` → CP 包名 `abbrev`）

零堆分配的 CP 原生唯一无歧义前缀缩写生成器，Clean-Room ISC 实现对标 npm 官方经典包 `abbrev`（1.00 亿+/周下载）。

按字典序排序去重并基于相邻最长公共前缀（LCP）计算全部唯一缩写映射表，支持精确匹配优先、最短唯一前缀查询与无歧义前缀解析。

## 核心 API

- `AbbrevEntry` — 缩写映射条目（`abbrev: string`, `full: string`）
- `abbrev(words: []const string, out: []AbbrevEntry): usize` — 计算词集全部唯一无歧义 `(abbrev, full)` 映射表并写入 `out`
- `shortestAbbrev(words: []const string, target: string): string?` — 返回 `target` 在词集中的最短唯一前缀（精确匹配保底）
- `resolveAbbrev(words: []const string, prefix: string): string?` — 解析前缀为唯一目标词（精确匹配优先，歧义或无匹配返回 `null`）

## 致谢

Clean-Room ISC 实现，API 语义对标开源生态 [`npm/abbrev-js`](https://github.com/npm/abbrev-js)。
