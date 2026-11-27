[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/iconv-lite

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:iconv-lite` ([https://github.com/ashtuchkin/iconv-lite](https://github.com/ashtuchkin/iconv-lite))

Zero-allocation character encoding converter for CP — clean-room MIT parity with npm `iconv-lite` (442M+/wk).

纯 CP 零堆分配字符编码转换引擎，支持 UTF-8、ASCII、Latin1（ISO-8859-1）、Windows-1252（完整 27 个 `0x80..0x9F` 扩展码点映射）、UTF-16LE/UTF-16BE（完整代理对与 BOM 剥离/注入）。

## Install / 安装

```bash
cap add @cap/iconv-lite
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Charset` | `enum` | `Utf8`, `Ascii`, `Latin1`, `Win1252`, `Utf16Le`, `Utf16Be`, `Unknown` |
| `parseCharset` | `(name: string): Charset` | Case-insensitive charset parser normalizing `-`/`_` |
| `encodingExists` | `(name: string): bool` | Returns `true` if the charset name is supported |
| `decodeToUtf8` | `(data: []const u8, charset: Charset, strip_bom: bool, out: []u8): usize` | Decode bytes from `charset` to UTF-8 |
| `encodeFromUtf8` | `(utf8: string, charset: Charset, add_bom: bool, out: []u8): usize` | Encode UTF-8 string into `charset` (`?` fallback) |

## Usage / 使用示例

```cp
import { parseCharset, decodeToUtf8, encodeFromUtf8 } from "@cap/iconv-lite";

const raw: [3]u8 = [0x80, 0x93, 0x99]; // €, “, ™ in Windows-1252
let buf: [32]u8 = undefined;
const n = decodeToUtf8(raw[0..3], parseCharset("windows-1252"), true, buf[0..32]);
```
