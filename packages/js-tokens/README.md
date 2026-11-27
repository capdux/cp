[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:js-tokens` ([https://github.com/lydell/js-tokens](https://github.com/lydell/js-tokens)) | 许可：`MIT`

# `js-tokens`

零堆分配纯 CP JavaScript/TypeScript 词法分词器，对标 npm **#61** `js-tokens`（真实周下载 **3.81 亿/周**，Babel/ESLint/React/Jest 底层依赖）。

## 核心 API

- `nextJsToken(src: string, pos: usize): JsToken?` — 零拷贝逐个扫描下一个 Token
- `tokenizeJs(src: string, out: []JsToken): usize` — 批量分词至调用方切片
- `countJsTokens(src: string): usize` — 零分配统计 Token 总数
