[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bumpalo

Zero-allocation alignment-aware bump arena allocator (BumpArena, BumpSlice) with checkpoint/rewind and overflow protection (Rust bumpalo v3 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bumpalo` ([https://github.com/fitzgen/bumpalo](https://github.com/fitzgen/bumpalo))

| Metadata | Value |
| --- | --- |
| Package | `bumpalo` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bumpalo
```

```cp
import {
    BumpSlice,
    BumpArena,
    bmpIsValidAlign,
    bmpAlignUp,
    bmpHandleValid,
} from "bumpalo";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BumpSlice` | `class` | Fields: `offset: usize, len: usize` · Methods: `empty, isEmpty, end` |
| `BumpArena` | `class` | Fields: `buf: [4096]u8, cursor: usize, alloc_count: usize` · Methods: `new, capacity, allocatedBytes, remaining, allocationCount, reset, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bmpIsValidAlign` | `(alignment: usize): bool` | Invokes `bmpIsValidAlign` and returns `bool`. |
| `bmpAlignUp` | `(offset: usize, alignment: usize): usize?` | Invokes `bmpAlignUp` and returns `usize?`. |
| `bmpHandleValid` | `(handle: BumpSlice, cursor: usize): bool` | Invokes `bmpHandleValid` and returns `bool`. |

## Minimal Example

```cp
import { bmpIsValidAlign } from "bumpalo";

pub fn main() {
    let result = bmpIsValidAlign(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
