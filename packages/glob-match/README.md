[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# glob-match

Zero-allocation, anti-backtracking path glob and brace alternation matching engine for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:glob-match` ([https://github.com/devongovett/glob-match](https://github.com/devongovett/glob-match))

| Metadata | Value |
| --- | --- |
| Package | `glob-match` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add glob-match
```

```cp
import {
    GlobSpan,
    gbmFindClassClose,
    gbmMatchCharClass,
    gbmMatchCore,
    gbmFindBracePair,
    gbmCopySlice,
    gbmMatchWithBraces,
    globMatch,
} from "glob-match";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GlobSpan` | `class` | Fields: `start: usize, end: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `gbmFindClassClose` | `(pattern: string, open_idx: usize): usize` | Invokes `gbmFindClassClose` and returns `usize`. |
| `gbmMatchCharClass` | `(pattern: string, open_idx: usize, close_idx: usize, ch: u8): bool` | Invokes `gbmMatchCharClass` and returns `bool`. |
| `gbmMatchCore` | `(pattern: string, path: string, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool` | Invokes `gbmMatchCore` and returns `bool`. |
| `gbmFindBracePair` | `(pattern: string, open_out: &mut usize, close_out: &mut usize): bool` | Invokes `gbmFindBracePair` and returns `bool`. |
| `gbmCopySlice` | `(dst: []u8, pos: usize, src: string, s: usize, e: usize): usize` | Invokes `gbmCopySlice` and returns `usize`. |
| `gbmMatchWithBraces` | `(pattern: string, path: string, depth: usize, steps: &mut usize, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool` | Invokes `gbmMatchWithBraces` and returns `bool`. |
| `globMatch` | `(pattern: string, path: string): bool` | Invokes `globMatch` and returns `bool`. |
| `globMatchAny` | `(patterns: &[]string, path: string): bool` | Invokes `globMatchAny` and returns `bool`. |
| `globMatchCaptures` | `(pattern: string, path: string, out_caps: []GlobSpan): usize` | Invokes `globMatchCaptures` and returns `usize`. |
| `isStaticGlob` | `(pattern: string): bool` | Checks predicate `isStaticGlob` and returns `bool`. |

## Minimal Example

```cp
import { isStaticGlob } from "glob-match";

pub fn main() {
    let result = isStaticGlob("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
