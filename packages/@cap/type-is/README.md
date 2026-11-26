# @cap/type-is

Zero-allocation HTTP `Content-Type` MIME matcher supporting parameter stripping, short aliases (`"json"`, `"urlencoded"`, `"multipart"`, etc.), wildcards (`"image/*"`, `"*/json"`), and RFC 6839 structured suffixes (`"+json"`, `"+xml"`) for CP — clean-room MIT parity with npm `type-is` (227M/wk).

纯 CP 零堆分配 HTTP `Content-Type` MIME 类型判断引擎，适用于 Web 框架请求体解析分发与内容类型校验。

## Install / 安装

```bash
cap add @cap/type-is
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `stripParams` | `(content_type: []const u8) -> []const u8` | Zero-copy strip `; charset=utf-8` & whitespace |
| `normalizeType` | `(short_or_mime: []const u8) -> []const u8` | Expand short aliases (`json`, `urlencoded`, `multipart`, `html`, `text`, `xml`, `js`, `css`, `bin`) |
| `mimeMatch` | `(content_type: []const u8, pattern: []const u8) -> bool` | Case-insensitive MIME / alias / wildcard / `+suffix` matcher |
| `matchAny` | `(content_type: []const u8, patterns: []const []const u8) -> i64` | First matching pattern index or `-1` |

## Usage / 使用示例

```cp
import { mimeMatch, matchAny } from "@cap/type-is";

const ok = mimeMatch("application/problem+json; charset=utf-8", "+json"); // true
```
