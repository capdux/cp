[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# iconv-lite

Zero-allocation character encoding converter supporting UTF-8, ASCII, Latin1, Windows-1252, UTF-16LE/BE with BOM and surrogate pairs for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:iconv-lite` ([https://github.com/ashtuchkin/iconv-lite](https://github.com/ashtuchkin/iconv-lite))

| Metadata | Value |
| --- | --- |
| Package | `iconv-lite` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add iconv-lite
```

```cp
import {
    Charset,
    IcvUtf8Step,
    icvStrEq,
    parseCharset,
    encodingExists,
    icvWin1252ToUnicode,
    icvUnicodeToWin1252,
    icvWriteUtf8,
} from "iconv-lite";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `Charset` | `Utf8, Ascii, Latin1, Win1252, Utf16Le, Utf16Be, Unknown` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `IcvUtf8Step` | `class` | Fields: `cp: u32, len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `icvStrEq` | `(a: string, b: string): bool` | Invokes `icvStrEq` and returns `bool`. |
| `parseCharset` | `(name: string): Charset` | Parses input via `parseCharset` and returns `Charset`. |
| `encodingExists` | `(name: string): bool` | Invokes `encodingExists` and returns `bool`. |
| `icvWin1252ToUnicode` | `(b: u8): u32` | Invokes `icvWin1252ToUnicode` and returns `u32`. |
| `icvUnicodeToWin1252` | `(cp: u32): u8?` | Invokes `icvUnicodeToWin1252` and returns `u8?`. |
| `icvWriteUtf8` | `(out: []u8, pos: usize, cp: u32): usize` | Invokes `icvWriteUtf8` and returns `usize`. |
| `icvReadUtf8` | `(s: string, pos: usize): IcvUtf8Step` | Invokes `icvReadUtf8` and returns `IcvUtf8Step`. |
| `icvReadU16` | `(data: &[]u8, pos: usize, is_le: bool): u16` | Invokes `icvReadU16` and returns `u16`. |
| `icvWriteU16` | `(out: []u8, pos: usize, val: u16, is_le: bool): usize` | Invokes `icvWriteU16` and returns `usize`. |
| `decodeToUtf8` | `(data: &[]u8, charset: Charset, strip_bom: bool, out: []u8): usize` | Parses input via `decodeToUtf8` and returns `usize`. |
| `encodeFromUtf8` | `(utf8: string, charset: Charset, add_bom: bool, out: []u8): usize` | Serializes/formats data via `encodeFromUtf8` returning `usize`. |

## Minimal Example

```cp
import { parseCharset } from "iconv-lite";

pub fn main() {
    let result = parseCharset("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
