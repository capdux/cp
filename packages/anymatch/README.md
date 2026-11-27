[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# anymatch

Zero-heap file path & glob matcher with negation support, composing normalize-path and is-glob in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:anymatch` ([https://github.com/micromatch/anymatch](https://github.com/micromatch/anymatch))

| Metadata | Value |
| --- | --- |
| Package | `anymatch` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add anymatch
```

```cp
import {
    startsWithDirPrefix,
    globMatchAt,
    matchPattern,
    anymatchIndex,
    anymatch,
    countGlobPatterns,
} from "anymatch";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `startsWithDirPrefix` | `(path: &[]u8, prefix: &[]u8): bool` | Invokes `startsWithDirPrefix` and returns `bool`. |
| `globMatchAt` | `(pat: &[]u8, pi_in: usize, txt: &[]u8, ti_in: usize): bool` | Invokes `globMatchAt` and returns `bool`. |
| `matchPattern` | `(pattern: string, test_path: string): bool` | Invokes `matchPattern` and returns `bool`. |
| `anymatchIndex` | `(patterns: &[]string, test_path: string): i64` | Invokes `anymatchIndex` and returns `i64`. |
| `anymatch` | `(patterns: &[]string, test_path: string): bool` | Invokes `anymatch` and returns `bool`. |
| `countGlobPatterns` | `(patterns: &[]string): usize` | Invokes `countGlobPatterns` and returns `usize`. |

## Minimal Example

```cp
import { matchPattern } from "anymatch";

pub fn main() {
    let result = matchPattern("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
