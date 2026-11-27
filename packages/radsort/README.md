[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# radsort

Zero-allocation O(N) 8-bit LSD radix sort for u32, i32, u64, i64, and IEEE-754 f64 with pass skipping for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:radsort` ([https://github.com/JakubValtar/radsort](https://github.com/JakubValtar/radsort))（原库名 `radsort` → CP 包名 `radsort`）

| Metadata | Value |
| --- | --- |
| Package | `radsort` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add radsort
```

```cp
import {
    i32ToSortable,
    i64ToSortable,
    f64ToSortable,
    sortU32,
    sortI32,
    sortU64,
    sortI64,
    sortF64,
} from "radsort";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `i32ToSortable` | `(v: i32): u32` | Invokes `i32ToSortable` and returns `u32`. |
| `i64ToSortable` | `(v: i64): u64` | Invokes `i64ToSortable` and returns `u64`. |
| `f64ToSortable` | `(v: f64): u64` | Invokes `f64ToSortable` and returns `u64`. |
| `sortU32` | `(data: []u32, scratch: []u32): bool` | Invokes `sortU32` and returns `bool`. |
| `sortI32` | `(data: []i32, scratch: []i32): bool` | Invokes `sortI32` and returns `bool`. |
| `sortU64` | `(data: []u64, scratch: []u64): bool` | Invokes `sortU64` and returns `bool`. |
| `sortI64` | `(data: []i64, scratch: []i64): bool` | Invokes `sortI64` and returns `bool`. |
| `sortF64` | `(data: []f64, scratch: []f64): bool` | Invokes `sortF64` and returns `bool`. |
| `isSortedI64` | `(data: &[]i64): bool` | Checks predicate `isSortedI64` and returns `bool`. |
| `isSortedU64` | `(data: &[]u64): bool` | Checks predicate `isSortedU64` and returns `bool`. |

## Minimal Example

```cp
import { isSortedI64 } from "radsort";

pub fn main() {
    let result = isSortedI64(/* data: &[]i64 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
