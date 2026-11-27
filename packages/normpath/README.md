[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# normpath

Zero-heap cross-platform lexical path normalization (POSIX, Windows drive/UNC/verbatim) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:normpath` ([https://github.com/dylni/normpath](https://github.com/dylni/normpath))

| Metadata | Value |
| --- | --- |
| Package | `normpath` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add normpath
```

```cp
import {
    npIsAsciiAlpha,
    npToUpper,
    npIsWinSep,
    npWriteByte,
    npWriteSlice,
    npSegEqDot,
    npSegEqDotDot,
    npNormalizePosixTwo,
} from "normpath";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `npIsAsciiAlpha` | `(b: u8): bool` | Invokes `npIsAsciiAlpha` and returns `bool`. |
| `npToUpper` | `(b: u8): u8` | Invokes `npToUpper` and returns `u8`. |
| `npIsWinSep` | `(b: u8): bool` | Invokes `npIsWinSep` and returns `bool`. |
| `npWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `npWriteByte` and returns `usize`. |
| `npWriteSlice` | `(out: []u8, pos: usize, src: string): usize` | Invokes `npWriteSlice` and returns `usize`. |
| `npSegEqDot` | `(src: string, start: usize, len: usize): bool` | Invokes `npSegEqDot` and returns `bool`. |
| `npSegEqDotDot` | `(src: string, start: usize, len: usize): bool` | Invokes `npSegEqDotDot` and returns `bool`. |
| `npNormalizePosixTwo` | `(p1: string, p2: string, hasP2: bool, out: []u8): usize` | Invokes `npNormalizePosixTwo` and returns `usize`. |
| `normalizePosix` | `(path: string, out: []u8): usize` | Invokes `normalizePosix` and returns `usize`. |
| `joinNormalized` | `(base: string, rel: string, out: []u8): usize` | Invokes `joinNormalized` and returns `usize`. |
| `isVerbatimWindows` | `(path: string): bool` | Checks predicate `isVerbatimWindows` and returns `bool`. |
| `stripVerbatimPrefix` | `(path: string): string` | Invokes `stripVerbatimPrefix` and returns `string`. |
| `npHasUncTag` | `(path: string, off: usize): bool` | Invokes `npHasUncTag` and returns `bool`. |
| `normalizeWindows` | `(path: string, out: []u8): usize` | Invokes `normalizeWindows` and returns `usize`. |
| `npWinIsAbsolute` | `(path: string): bool` | Invokes `npWinIsAbsolute` and returns `bool`. |
| `joinNormalizedWindows` | `(base: string, rel: string, out: []u8): usize` | Invokes `joinNormalizedWindows` and returns `usize`. |
| `hasParentTraversal` | `(path: string): bool` | Checks predicate `hasParentTraversal` and returns `bool`. |
| `escapesRoot` | `(path: string): bool` | Invokes `escapesRoot` and returns `bool`. |
| `isNormalized` | `(path: string): bool` | Checks predicate `isNormalized` and returns `bool`. |

## Minimal Example

```cp
import { isVerbatimWindows } from "normpath";

pub fn main() {
    let result = isVerbatimWindows("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
