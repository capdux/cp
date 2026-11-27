[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# suffix

Zero-heap static Suffix Array + Kasai O(N) LCP array with O(M log N) binary search, occurrence counting, and longest repeated substring detection.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:suffix` ([https://github.com/BurntSushi/suffix](https://github.com/BurntSushi/suffix))（原库名 `suffix` → CP 包名 `suffix`）

| Metadata | Value |
| --- | --- |
| Package | `suffix` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add suffix
```

```cp
import { LrsResult, SuffixArray, compareSuffixes } from "suffix";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LrsResult` | `class` | Fields: `start: usize, len: usize, found: bool` |
| `SuffixArray` | `class` | Fields: `text: [256]u8, sa: [256]u16, lcp: [256]u16, text_len: usize` · Methods: `build, len, suffixAt, lcpAt, comparePrefix, lowerBound, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `compareSuffixes` | `(buf: &[]u8, n: usize, a: usize, b: usize): i32` | Invokes `compareSuffixes` and returns `i32`. |

## Minimal Example

```cp
import { compareSuffixes } from "suffix";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = compareSuffixes(out_buf[0..256], 0, 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
