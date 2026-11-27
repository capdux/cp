[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# kmp

Zero-allocation O(N+M) Knuth-Morris-Pratt (KMP) substring search and LPS failure function engine for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:kmp` ([https://github.com/magiclen/kmp](https://github.com/magiclen/kmp))（原库名 `kmp` → CP 包名 `kmp`）

| Metadata | Value |
| --- | --- |
| Package | `kmp` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add kmp
```

```cp
import {
    computeLps,
    findFirst,
    findAll,
    countMatches,
    contains,
} from "kmp";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `computeLps` | `(pattern: &[]u8, out_lps: []usize): usize` | Invokes `computeLps` and returns `usize`. |
| `findFirst` | `(haystack: &[]u8, needle: &[]u8): i64` | Queries/retrieves data via `findFirst` returning `i64`. |
| `findAll` | `(haystack: &[]u8, needle: &[]u8, overlapping: bool, out_offsets: []usize): usize` | Queries/retrieves data via `findAll` returning `usize`. |
| `countMatches` | `(haystack: &[]u8, needle: &[]u8, overlapping: bool): usize` | Invokes `countMatches` and returns `usize`. |
| `contains` | `(haystack: &[]u8, needle: &[]u8): bool` | Invokes `contains` and returns `bool`. |

## Minimal Example

```cp
import { computeLps } from "kmp";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = computeLps(out_buf[0..256], /* out_lps: []usize */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
