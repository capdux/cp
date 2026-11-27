[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# litrs

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:litrs` ([https://github.com/LukasKalbertodt/litrs](https://github.com/LukasKalbertodt/litrs))

Zero-allocation Rust literal lexer, validator, and string unescaper for CP — clean-room parity with Rust `litrs` (232M+ downloads on crates.io).

纯 CP 零堆分配 Rust 字面量词法解析与检验器，完整支持布尔值、十/十六/二/八进制整数（含 `_` 分隔符与类型后缀）、浮点数（含指数与 `f32`/`f64` 后缀）、字符/字节与转义、普通字符串与原始字符串 `r#"..."#` / `br#"..."#`，以及字符串反转义解码。

## Install / 安装

```bash
cap add litrs
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `LiteralKind` | `enum: u8 { Invalid, Bool, Integer, Float, Char, Byte, String, ByteString }` | Classified literal kind |
| `ParsedLiteral` | `struct { kind, raw, suffix, int_val, bool_val, is_raw_string, hash_count }` | Parsed literal metadata & value |
| `parseLiteral` | `(src: string) -> ParsedLiteral` | Lexes and validates a Rust literal |
| `unescapeString` | `(lit_raw: string, out: []u8) -> i64` | Unescapes normal/raw/byte string literals into `out` (`-1` on error) |

## Usage / 使用示例

```cp
import { parseLiteral, unescapeString, LiteralKind } from "litrs";

const lit = parseLiteral("0xFF_00u32"); // kind == Integer, int_val == 65280, suffix == "u32"
const raw = parseLiteral("r#\"hello\\nworld\"#"); // kind == String, is_raw_string == true, hash_count == 1

let out: [64]u8 = undefined;
const n = unescapeString("\"hi\\n\\x21\"", out[0..64]); // n == 4 ("hi\n!")
```
