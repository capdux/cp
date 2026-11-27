[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# glob-to-regexp

Zero-allocation POSIX/glob wildcard to ECMAScript/PCRE regular expression converter with extended & globstar modes in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:glob-to-regexp` ([https://github.com/fitzgen/glob-to-regexp](https://github.com/fitzgen/glob-to-regexp))（原库名 `glob-to-regexp` → CP 包名 `glob-to-regexp`）

| Metadata | Value |
| --- | --- |
| Package | `glob-to-regexp` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add glob-to-regexp
```

```cp
import {
    GlobOptions,
    pushByte,
    pushSlice,
    isEscapeMeta,
    globToRegex,
    globToRegexDefault,
} from "glob-to-regexp";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GlobOptions` | `class` | Fields: `extended: bool, globstar: bool, anchored: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pushByte` | `(dst: []u8, pos: usize, b: u8): i64` | Mutates or appends state via `pushByte`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: &[]u8): i64` | Mutates or appends state via `pushSlice`. |
| `isEscapeMeta` | `(b: u8): bool` | Checks predicate `isEscapeMeta` and returns `bool`. |
| `globToRegex` | `(glob: &[]u8, opts: GlobOptions, dst: []u8): i64` | Invokes `globToRegex` and returns `i64`. |
| `globToRegexDefault` | `(glob: &[]u8, dst: []u8): i64` | Invokes `globToRegexDefault` and returns `i64`. |

## Minimal Example

```cp
import { isEscapeMeta } from "glob-to-regexp";

pub fn main() {
    let result = isEscapeMeta(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
