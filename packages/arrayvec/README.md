[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# arrayvec

Zero-allocation stack-backed fixed-capacity containers: ArrayString64, ArrayString256, ArrayVecI64, ArrayVecU64 with O(1) swapRemove and truncating push (Rust arrayvec parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:arrayvec` ([https://github.com/bluss/arrayvec](https://github.com/bluss/arrayvec))

| Metadata | Value |
| --- | --- |
| Package | `arrayvec` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add arrayvec
```

```cp
import { ArrayString64, ArrayString256, ArrayVecI64, ArrayVecU64 } from "arrayvec";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ArrayString64` | `class` | Fields: `buf: [STR64_CAP]u8, count: usize` · Methods: `init, fromStr, len, capacity, remaining, isEmpty, +8 more` |
| `ArrayString256` | `class` | Fields: `buf: [STR256_CAP]u8, count: usize` · Methods: `init, fromStr, len, capacity, remaining, isEmpty, +8 more` |
| `ArrayVecI64` | `class` | Fields: `items: [VEC64_CAP]i64, count: usize` · Methods: `init, len, isEmpty, isFull, tryPush, pop, +7 more` |
| `ArrayVecU64` | `class` | Fields: `items: [VEC64_CAP]u64, count: usize` · Methods: `init, len, isEmpty, isFull, tryPush, pop, +7 more` |

## Minimal Example

```cp
import { ArrayString64 } from "arrayvec";

pub fn main() {
    // Refer to src/index.d.cp for ArrayString64 fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
