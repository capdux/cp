# @cap/content-type

零堆分配的 RFC 9110 / RFC 6838 HTTP `Content-Type` 媒体类型解析、参数提取与安全格式化引擎 — 洁净室 MIT 实现，对标 npm `content-type` 与 `media-typer`。

Zero-allocation RFC 9110 / RFC 6838 HTTP `Content-Type` media type parser, parameter extractor, and formatter for CP — clean-room MIT parity with npm `content-type` and `media-typer`.

## 特性 / Features

- **严格 RFC 9110 词法校验 / Strict RFC 9110 validation**：完整校验 `tchar` token 字符集，拒绝空类型、多斜杠、未闭合引号及非法参数。
- **RFC 6838 `+suffix` 结构化后缀提取 / Structured `+suffix` support**：自动提取 `application/problem+json` → `suffix = "json"`、`application/vnd.api+json` 等。
- **引号参数与转义安全 / Quoted-string safe**：支持参数值内嵌分号 `;` 与 `\"` 转义，`formatContentType` 自动对非 token 值加引号并转义。
- **零堆分配 / Zero heap allocation**：所有解析切片指向原头缓冲区，格式化直写调用方 `out: []u8`。

## 安装 / Install

```bash
cap add @cap/content-type
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `MediaParam` | `{ key: string, value: string }` 参数键值对 |
| `ContentType` | `{ ok, type_str, subtype, suffix, essence, params[16], param_count }` + `charset()` / `boundary()` / `getParam(key)` |
| `parseContentType(header)` | 解析 HTTP `Content-Type` 头 |
| `isJsonContentType(header)` | 判断是否为 `application/json`、`text/json` 或 `*/*+json` |
| `isTextContentType(header)` | 判断是否为 `text/*` |
| `isMultipartContentType(header)` | 判断是否为 `multipart/*` |
| `formatContentType(ct, out)` | 零分配格式化为规范小写 RFC 9110 `Content-Type` 字符串 |

## 使用示例 / Usage

```cp
import {
    parseContentType,
    formatContentType,
    isJsonContentType,
} from "@cap/content-type";

const ct = parseContentType("application/problem+json; charset=utf-8");
// ct.ok == true, ct.type_str == "application", ct.subtype == "problem+json"
// ct.suffix == "json", ct.charset().? == "utf-8"

const is_json = isJsonContentType("application/vnd.api+json; charset=utf-8"); // true

let buf: [256]u8 = undefined;
const normalized = formatContentType(ct, buf[0..256]);
// => "application/problem+json; charset=utf-8"
```

## License

MIT
