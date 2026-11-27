[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# d3-format

Zero-heap D3/Python-style number format specifier parser and formatter for CP (clean-room ISC implementation of npm d3-format).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-format` ([https://github.com/d3/d3-format](https://github.com/d3/d3-format))（原库名 `d3-format` → CP 包名 `d3-format`）

| Metadata | Value |
| --- | --- |
| Package | `d3-format` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add d3-format
```

```cp
import {
    FormatSpec,
    d3fIsAlign,
    d3fIsType,
    parseFormatSpec,
    d3fPow10F64,
    d3fPow10U64,
    d3fWriteU64Base,
    d3fWriteFixed,
} from "d3-format";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FormatSpec` | `class` | Fields: `valid: bool, fill: u8, @"align": u8, sign: u8, symbol: u8, zero: bool, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `d3fIsAlign` | `(ch: u8): bool` | Invokes `d3fIsAlign` and returns `bool`. |
| `d3fIsType` | `(ch: u8): bool` | Invokes `d3fIsType` and returns `bool`. |
| `parseFormatSpec` | `(spec: string): FormatSpec` | Parses input via `parseFormatSpec` and returns `FormatSpec`. |
| `d3fPow10F64` | `(exp: i32): f64` | Invokes `d3fPow10F64` and returns `f64`. |
| `d3fPow10U64` | `(exp: i32): u64` | Invokes `d3fPow10U64` and returns `u64`. |
| `d3fWriteU64Base` | `(val: u64, base: u64, upper: bool, comma: bool, out: []u8): usize` | Invokes `d3fWriteU64Base` and returns `usize`. |
| `d3fWriteFixed` | `(abs_val: f64, prec_in: i32, comma: bool, trim: bool, out: []u8): usize` | Invokes `d3fWriteFixed` and returns `usize`. |
| `d3fAppendSiSuffix` | `(exp3: i32, out: []u8, pos: usize): usize` | Invokes `d3fAppendSiSuffix` and returns `usize`. |
| `d3fDisplayLen` | `(slice: &[]u8): usize` | Invokes `d3fDisplayLen` and returns `usize`. |
| `d3fCopySlice` | `(dst: []u8, pos: usize, src: &[]u8): usize` | Invokes `d3fCopySlice` and returns `usize`. |
| `formatNumber` | `(spec: string, value: f64, out: []u8): usize` | Serializes/formats data via `formatNumber` returning `usize`. |

## Minimal Example

```cp
import { parseFormatSpec } from "d3-format";

pub fn main() {
    let result = parseFormatSpec("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
