[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# litrs

Zero-allocation Rust literal lexer, validator, and string unescaper (Rust litrs parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:litrs` ([https://github.com/LukasKalbertodt/litrs](https://github.com/LukasKalbertodt/litrs))

| Metadata | Value |
| --- | --- |
| Package | `litrs` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add litrs
```

```cp
import {
    LiteralKind,
    ParsedLiteral,
    ltrInvalid,
    ltrStrEq,
    ltrIsIdentStart,
    ltrIsIdentCont,
    ltrIsValidSuffix,
    ltrHexVal,
} from "litrs";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `LiteralKind` | `Invalid, Bool, Integer, Float, Char, Byte, String, ByteString` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedLiteral` | `class` | Fields: `kind: LiteralKind, raw: string, suffix: string, int_val: u64, bool_val: bool, is_raw_string: bool, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ltrInvalid` | `(src: string): ParsedLiteral` | Invokes `ltrInvalid` and returns `ParsedLiteral`. |
| `ltrStrEq` | `(a: string, b: string): bool` | Invokes `ltrStrEq` and returns `bool`. |
| `ltrIsIdentStart` | `(c: u8): bool` | Invokes `ltrIsIdentStart` and returns `bool`. |
| `ltrIsIdentCont` | `(c: u8): bool` | Invokes `ltrIsIdentCont` and returns `bool`. |
| `ltrIsValidSuffix` | `(s: string): bool` | Invokes `ltrIsValidSuffix` and returns `bool`. |
| `ltrHexVal` | `(c: u8): i32` | Invokes `ltrHexVal` and returns `i32`. |
| `ltrDigitVal` | `(c: u8, radix: u64): i32` | Invokes `ltrDigitVal` and returns `i32`. |
| `ltrParseRawString` | `(src: string, start: usize, kind: LiteralKind): ParsedLiteral` | Invokes `ltrParseRawString` and returns `ParsedLiteral`. |
| `ltrParseQuotedString` | `(src: string, quote_pos: usize, kind: LiteralKind): ParsedLiteral` | Invokes `ltrParseQuotedString` and returns `ParsedLiteral`. |
| `ltrParseCharOrByte` | `(src: string, quote_pos: usize, is_byte: bool): ParsedLiteral` | Invokes `ltrParseCharOrByte` and returns `ParsedLiteral`. |
| `ltrParseNumber` | `(src: string): ParsedLiteral` | Invokes `ltrParseNumber` and returns `ParsedLiteral`. |
| `parseLiteral` | `(src: string): ParsedLiteral` | Parses input via `parseLiteral` and returns `ParsedLiteral`. |
| `ltrEncodeUtf8` | `(cp: u32, out: []u8, pos: usize): usize?` | Invokes `ltrEncodeUtf8` and returns `usize?`. |
| `unescapeString` | `(lit_raw: string, out: []u8): i64` | Invokes `unescapeString` and returns `i64`. |

## Minimal Example

```cp
import { parseLiteral } from "litrs";

pub fn main() {
    let result = parseLiteral("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
