[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tinyvec

Zero-heap 100%-safe inline stack vector (TinyVecI64 & TinyVecU32) with insert, swapRemove, dedup, reverse, and binarySearch (clean-room port of Rust tinyvec)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tinyvec` ([https://github.com/Lokathor/tinyvec](https://github.com/Lokathor/tinyvec))

| Metadata | Value |
| --- | --- |
| Package | `tinyvec` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tinyvec
```

```cp
import { TinyVecI64, TinyVecU32 } from "tinyvec";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TinyVecI64` | `class` | Fields: `data: [64]i64, used: usize` · Methods: `new, len, capacity, isEmpty, isFull, get, +14 more` |
| `TinyVecU32` | `class` | Fields: `data: [64]u32, used: usize` · Methods: `new, len, isEmpty, push, pop, get, +2 more` |

## Minimal Example

```cp
import { TinyVecI64 } from "tinyvec";

pub fn main() {
    // Refer to src/index.d.cp for TinyVecI64 fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
