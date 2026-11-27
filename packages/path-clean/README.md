[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# path-clean

Zero-heap lexical POSIX path cleaner following Go filepath.Clean / Rust path-clean semantics in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:path-clean` ([https://github.com/danreeves/path-clean](https://github.com/danreeves/path-clean))（原库名 `path-clean` → CP 包名 `path-clean`）

| Metadata | Value |
| --- | --- |
| Package | `path-clean` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add path-clean
```

```cp
import {
    pclByteAt,
    pclCleanTwo,
    cleanPath,
    cleanJoin,
    isClean,
} from "path-clean";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pclByteAt` | `(a: string, b: string, has_sep: bool, idx: usize): u8` | Invokes `pclByteAt` and returns `u8`. |
| `pclCleanTwo` | `(a: string, b: string, has_sep: bool, out: []u8): usize` | Invokes `pclCleanTwo` and returns `usize`. |
| `cleanPath` | `(path: string, out: []u8): usize` | Invokes `cleanPath` and returns `usize`. |
| `cleanJoin` | `(a: string, b: string, out: []u8): usize` | Invokes `cleanJoin` and returns `usize`. |
| `isClean` | `(path: string): bool` | Checks predicate `isClean` and returns `bool`. |

## Minimal Example

```cp
import { isClean } from "path-clean";

pub fn main() {
    let result = isClean("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
