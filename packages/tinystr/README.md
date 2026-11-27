[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tinystr

Zero-allocation stack-inline fixed-capacity ASCII strings (TinyAsciiStr4, TinyAsciiStr8, TinyAsciiStr16) with u32/u64 packing and BCP-47 case transforms (ICU4X tinystr parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tinystr` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))

| Metadata | Value |
| --- | --- |
| Package | `tinystr` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tinystr
```

```cp
import {
    TinyAsciiStr4,
    TinyAsciiStr4Result,
    TinyAsciiStr8,
    TinyAsciiStr8Result,
    TinyAsciiStr16,
    TinyAsciiStr16Result,
    tsIsLower,
    tsIsUpper,
} from "tinystr";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TinyAsciiStr4` | `class` | Fields: `bytes: [4]u8` · Methods: `empty, fromBytes, fromStr, fromPackedU32, toPackedU32, len, +14 more` |
| `TinyAsciiStr4Result` | `class` | Fields: `ok: bool, value: TinyAsciiStr4, err: i64` |
| `TinyAsciiStr8` | `class` | Fields: `bytes: [8]u8` · Methods: `empty, fromBytes, fromStr, fromPackedU64, toPackedU64, len, +14 more` |
| `TinyAsciiStr8Result` | `class` | Fields: `ok: bool, value: TinyAsciiStr8, err: i64` |
| `TinyAsciiStr16` | `class` | Fields: `bytes: [16]u8` · Methods: `empty, fromBytes, fromStr, len, isEmpty, asBytes, +12 more` |
| `TinyAsciiStr16Result` | `class` | Fields: `ok: bool, value: TinyAsciiStr16, err: i64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `tsIsLower` | `(b: u8): bool` | Invokes `tsIsLower` and returns `bool`. |
| `tsIsUpper` | `(b: u8): bool` | Invokes `tsIsUpper` and returns `bool`. |
| `tsValidateBytes` | `(src: &[]u8, max_len: usize): i64` | Invokes `tsValidateBytes` and returns `i64`. |
| `tsValidatePacked` | `(buf: &[]u8): i64` | Invokes `tsValidatePacked` and returns `i64`. |
| `tsSliceLen` | `(buf: &[]u8): usize` | Invokes `tsSliceLen` and returns `usize`. |
| `tsCopyOut` | `(src: &[]u8, n: usize, out: []u8): usize` | Invokes `tsCopyOut` and returns `usize`. |
| `tsSliceCmp` | `(a: &[]u8, b: &[]u8): i32` | Invokes `tsSliceCmp` and returns `i32`. |
| `tsAllAlpha` | `(buf: &[]u8, n: usize): bool` | Invokes `tsAllAlpha` and returns `bool`. |
| `tsAllAlnum` | `(buf: &[]u8, n: usize): bool` | Invokes `tsAllAlnum` and returns `bool`. |
| `tsAllDigit` | `(buf: &[]u8, n: usize): bool` | Invokes `tsAllDigit` and returns `bool`. |
| `tsAllLower` | `(buf: &[]u8, n: usize): bool` | Invokes `tsAllLower` and returns `bool`. |
| `tsAllUpper` | `(buf: &[]u8, n: usize): bool` | Invokes `tsAllUpper` and returns `bool`. |

## Minimal Example

```cp
import { tsIsLower } from "tinystr";

pub fn main() {
    let result = tsIsLower(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
