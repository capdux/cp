[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# memmem

Zero-allocation byte-substring search engine with precompiled BMH + KMP Finder, rfind, and non-overlapping count (Rust memchr::memmem parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:memmem` ([https://github.com/BurntSushi/memchr](https://github.com/BurntSushi/memchr))

| Metadata | Value |
| --- | --- |
| Package | `memmem` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add memmem
```

```cp
import {
    Finder,
    findLongNeedle,
    find,
    rfind,
    contains,
    count,
} from "memmem";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Finder` | `class` | Fields: `needle_buf: [MAX_NEEDLE]u8, needle_len: usize, bad_char: [256]usize, lps: [MAX_NEEDLE]u16, is_periodic: bool` · Methods: `init, needleLen, findKmpFrom, findFrom, findIn, countIn` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `findLongNeedle` | `(haystack: &[]u8, needle: &[]u8): i64` | Queries/retrieves data via `findLongNeedle` returning `i64`. |
| `find` | `(haystack: &[]u8, needle: &[]u8): i64` | Queries/retrieves data via `find` returning `i64`. |
| `rfind` | `(haystack: &[]u8, needle: &[]u8): i64` | Invokes `rfind` and returns `i64`. |
| `contains` | `(haystack: &[]u8, needle: &[]u8): bool` | Invokes `contains` and returns `bool`. |
| `count` | `(haystack: &[]u8, needle: &[]u8): usize` | Invokes `count` and returns `usize`. |

## Minimal Example

```cp
import { findLongNeedle } from "memmem";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = findLongNeedle(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
