[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:http-content-range` ([https://github.com/briansmith/http-content-range](https://github.com/briansmith/http-content-range)) | 许可：`MIT OR Apache-2.0`

# `http-content-range`

零堆分配纯 CP RFC 9110 `Content-Range` 响应头解析与格式化引擎，完整覆盖 `bytes start-end/total`、`bytes start-end/*` 与 `bytes */total`，严格拦截非法区间与 `bytes */*`。

## 核心 API

- `parseContentRange(header: string): ContentRange`
- `formatContentRangeHeader(cr: ContentRange, out: []u8): usize`
- `ContentRange.isValid()` / `ContentRange.rangeLen()`
