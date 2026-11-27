[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fast-glob

Zero-heap pure CP fast-glob pattern set, dynamic pattern detector, and path escaper (clean-room MIT implementation of npm fast-glob v3)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-glob` ([https://github.com/mrmlnc/fast-glob](https://github.com/mrmlnc/fast-glob))

| Metadata | Value |
| --- | --- |
| Package | `fast-glob` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fast-glob
```

```cp
import {
    FastGlobOptions,
    GlobPatternSet,
    fglToLower,
    fglIsGlobMeta,
    fglNormalizeRelPath,
    fglPathDepth,
    fglHasDotSegment,
    fglLowerInto,
} from "fast-glob";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FastGlobOptions` | `class` | Fields: `dot: bool, deep: usize, only_files: bool, only_directories: bool, case_sensitive: bool` · Methods: `defaults` |
| `GlobPatternSet` | `class` | Fields: `opts: FastGlobOptions, includes: [16]string, include_count: usize, ignores: [16]string, ignore_count: usize` · Methods: `new, addPattern, addIgnore, matches` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fglToLower` | `(b: u8): u8` | Invokes `fglToLower` and returns `u8`. |
| `fglIsGlobMeta` | `(b: u8): bool` | Invokes `fglIsGlobMeta` and returns `bool`. |
| `fglNormalizeRelPath` | `(raw: string): string` | Invokes `fglNormalizeRelPath` and returns `string`. |
| `fglPathDepth` | `(path: string): usize` | Invokes `fglPathDepth` and returns `usize`. |
| `fglHasDotSegment` | `(path: string): bool` | Invokes `fglHasDotSegment` and returns `bool`. |
| `fglLowerInto` | `(src: string, dst: []u8): usize` | Invokes `fglLowerInto` and returns `usize`. |
| `fglMatchOne` | `(pattern: string, path: string, case_sensitive: bool): bool` | Invokes `fglMatchOne` and returns `bool`. |
| `isDynamicPattern` | `(pattern: string): bool` | Checks predicate `isDynamicPattern` and returns `bool`. |
| `escapePath` | `(path: string, out: []u8): usize` | Invokes `escapePath` and returns `usize`. |
| `convertPathToPattern` | `(path: string, out: []u8): usize` | Invokes `convertPathToPattern` and returns `usize`. |

## Minimal Example

```cp
import { isDynamicPattern } from "fast-glob";

pub fn main() {
    let result = isDynamicPattern("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
