[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/url-join

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:url-join` ([https://github.com/jfromaniello/url-join](https://github.com/jfromaniello/url-join))

零堆分配的智能 URL 拼接、斜杠规范化、相对路径 `.` / `..` 折叠与 Query/Hash 合并引擎 — 洁净室 MIT 实现，对标 npm `url-join` 与 `ufo`。

Zero-allocation smart URL joiner, slash/dot-segment path normalizer, query merger, and trailing-slash utilities for CP — clean-room MIT parity with npm `url-join` and `ufo`.

## 特性 / Features

- **全协议感知拼接 / Scheme-aware joining**：完整保留 `https://`、`http://`、`ws://`、`wss://`、`file:///`、`mailto:` 与协议相对 `//cdn.example.com`。
- **智能斜杠去重与 Query/Hash 合并 / Smart slashes & query/hash merging**：自动跳过空片段、消除重复斜杠、将多个 `?a=1` + `?b=2` 合并为 `?a=1&b=2`，并将 `#hash` 锚点移至最终末尾。
- **路径段规范化 / Dot-segment normalization**：`normalizeUrlPath` 折叠多余 `/`、`.` 与 `..` 段。
- **零堆分配 / Zero heap allocation**：全部写入调用方 `out: []u8` 缓冲区。

## 安装 / Install

```bash
cap add @cap/url-join
```

## API 参考 / API Reference

| 函数 / Function | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `urlJoin` | `(parts: []string, out: []u8) -> string` | 智能拼接 URL 片段，去重斜杠并合并 Query/Hash |
| `normalizeUrlPath` | `(path: string, out: []u8) -> string` | 折叠多余 `/`、`.` 与 `..` 路径段 |
| `withQuery` | `(base_url: string, key: string, value: string, out: []u8) -> string` | 追加查询参数（自动区分 `?`/`&` 并保留 `#hash`） |
| `withTrailingSlash` | `(url: string, out: []u8) -> string` | 为路径部分补齐尾斜杠（保留 `?query`/`#hash`） |
| `withoutTrailingSlash` | `(url: string, out: []u8) -> string` | 移除路径部分尾斜杠（保护根 `/` 与协议前缀） |

## 使用示例 / Usage

```cp
import {
    urlJoin,
    normalizeUrlPath,
    withQuery,
    withTrailingSlash,
    withoutTrailingSlash,
} from "@cap/url-join";

let buf: [512]u8 = undefined;
let parts: [4]string = ["https://capdux.org/", "/packages/", "@cap/lru", "?v=1"];
const url = urlJoin(parts[0..4], buf[0..512]);
// => "https://capdux.org/packages/@cap/lru?v=1"

const clean = normalizeUrlPath("/a/b/../c//./d", buf[0..512]);
// => "/a/c/d"

const q = withQuery("https://capdux.org/search?q=cp#top", "page", "2", buf[0..512]);
// => "https://capdux.org/search?q=cp&page=2#top"
```

## License

MIT
