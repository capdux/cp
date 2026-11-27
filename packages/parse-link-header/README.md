[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/link-header

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse-link-header` ([https://github.com/thlorenz/parse-link-header](https://github.com/thlorenz/parse-link-header))（原库名 `parse-link-header` → CP 包名 `@cap/link-header`）

Zero-allocation RFC 8288 HTTP `Link` header parser and builder with GitHub / Stripe / REST API pagination helpers for CP — clean-room MIT parity with npm `parse-link-header` / `http-link-header` and Rust `parse_link_header`.

零堆分配的 CP RFC 8288 HTTP `Link` 响应头解析与构建引擎，支持 GitHub/Stripe/REST API 分页链接解析与生成、引号内含逗号/分号、多 `rel` 关系、无引号参数值与畸形输入安全容错（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/link-header
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `LinkEntry` | `pub struct { url, rel, title, type_val, hreflang: string }` | 单条 RFC 8288 链接条目 |
| `LinkHeader` | `pub struct { entries: [32]LinkEntry, count: usize }` | 栈上最多 32 条链接集合，提供 `entryAt(i)`、`getByRel(rel)`、`nextUrl()`、`prevUrl()`、`firstUrl()`、`lastUrl()` |
| `parseLinkHeader` | `(header: string) -> LinkHeader` | 零拷贝解析 RFC 8288 `Link` 响应头（支持多 `rel`、引号内 `,`/`;`、大小写无关参数名与畸形跳过） |
| `formatLinkHeader` | `(entries: []LinkEntry, out: []u8) -> string` | 将 `LinkEntry` 切片序列化为标准 RFC 8288 `Link` 响应头 |
| `formatPaginationLinks` | `(base_url: string, page: usize, total_pages: usize, out: []u8) -> string` | 按当前页与总页数自动构建 `prev`/`next`/`last`/`first` 分页 `Link` 头 |

## Usage / 使用示例

```cp
import {
    LinkEntry,
    parseLinkHeader,
    formatLinkHeader,
    formatPaginationLinks,
} from "@cap/link-header";

// 1. Parse GitHub API pagination Link header
const raw = "<https://api.github.com/repos?page=2>; rel=\"next\", <https://api.github.com/repos?page=5>; rel=\"last\"";
const links = parseLinkHeader(raw);
const next_url = links.nextUrl(); // => "https://api.github.com/repos?page=2"
const last_url = links.lastUrl(); // => "https://api.github.com/repos?page=5"

// 2. Build REST pagination Link header
let out: [512]u8 = undefined;
const hdr = formatPaginationLinks("https://api.example.com/users?per_page=20", 3, 5, out[0..512]);
// => `<https://api.example.com/users?per_page=20&page=2>; rel="prev", <https://api.example.com/users?per_page=20&page=4>; rel="next", ...`
```

## License

MIT
