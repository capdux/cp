# @cap/querystring

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:querystring` ([https://github.com/sindresorhus/query-string](https://github.com/sindresorhus/query-string))

Zero-allocation URL query string parser, stringifier, and parameter extractor for CP — clean-room MIT parity with Node.js `querystring` and npm `query-string` v9. Composes `@cap/percent-encoding` with zero duplication.

纯 CP 零堆分配 URL 查询串解析与拼接库（对标 Node.js `querystring` 与 npm `query-string` v9），跨包复用 `@cap/percent-encoding` 实现 RFC/WHATWG URL 编解码。

## Install / 安装

```bash
cap add @cap/querystring
```

## Dependencies / 跨包复用

- `@cap/percent-encoding`: 复用 `componentSet`、`percentEncode` 与 `percentDecodePlus` 进行零堆分配百分号编解码

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `QueryEntry` | `struct { key: string, value: string }` | Raw query key-value entry |
| `escapeQuery` | `(raw: string, out: []u8): usize` | Percent-encode a query key or value |
| `unescapeQuery` | `(raw: string, out: []u8): usize` | Decode `+` and `%XX` sequences into `out` |
| `extractUrlQuery` | `(url: string): string` | Extract raw query substring between `?` and `#` |
| `parseQueryString` | `(input: string, sep: u8, eq: u8, out_entries: []QueryEntry): usize` | Parse query string skipping leading `?` and `#hash`, supporting custom `sep`/`eq` & bare flag keys |
| `stringifyQueryString` | `(entries: []const QueryEntry, sep: u8, eq: u8, out: []u8): usize` | Serialize entries with custom `sep`/`eq` and percent-encoding |
| `pickQueryParam` | `(url_or_query: string, target_key: string, out: []u8): usize?` | Find and decode a single parameter from a full URL or raw query string |

## Usage / 使用示例

```cp
import {
    QueryEntry,
    extractUrlQuery,
    parseQueryString,
    stringifyQueryString,
    pickQueryParam,
} from "@cap/querystring";

const q = extractUrlQuery("https://cp-lang.org/docs?lang=zh&page=2#top"); // "lang=zh&page=2"

let out: [128]u8 = undefined;
const maybeLen = pickQueryParam("https://cp-lang.org/search?q=hello+CP%21#res", "q", out[0..128]);
```
