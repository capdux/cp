[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/d3-time-format

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-time-format` ([https://github.com/d3/d3-time-format](https://github.com/d3/d3-time-format))（原库名 `d3-time-format` → CP 包名 `@cap/d3-time-format`）

零堆分配的 CP 原生 UTC `strftime` / `strptime` 日期格式化与解析库，Clean-Room ISC 实现对标 D3.js 顶流时间格式化库 `d3-time-format`（1.05 亿+/周下载）。

基于无符号民用历法算法（支持 1970 前后全范围闰年与毫秒精度），完整支持 `%Y`, `%m`, `%d`, `%H`, `%M`, `%S`, `%L`, `%b`, `%B`, `%a`, `%A`, `%j`, `%w`, `%Z`, `%%` 指令的双向格式化与严格校验解析。

## 核心 API

- `utcFormat(spec: string, unix_ms: i64, out: []u8): usize` — 将 UTC 毫秒时间戳按 `spec` 格式化写入 `out` 并返回写入字节数
- `utcParse(spec: string, input: string): i64?` — 按 `spec` 严格解析 UTC 时间字符串为毫秒时间戳，格式不匹配或日期越界返回 `null`

## 致谢

Clean-Room ISC 实现，API 语义对标开源生态 [`d3/d3-time-format`](https://github.com/d3/d3-time-format)。
