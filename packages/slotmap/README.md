[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# slotmap

Zero-allocation generational-index slot map (SlotKey, SlotMapI64, SecondaryMapI64) with O(1) free-list and strict ABA protection (Rust slotmap v1 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:slotmap` ([https://github.com/orlp/slotmap](https://github.com/orlp/slotmap))

| Metadata | Value |
| --- | --- |
| Package | `slotmap` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add slotmap
```

```cp
import {
    SlotKey,
    SlotMapI64,
    SecondaryMapI64,
    slmIsOccupiedVersion,
    slmAllocVersion,
    slmFreeVersion,
} from "slotmap";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SlotKey` | `class` | Fields: `idx: u32, version: u32` · Methods: `nullKey, isNull, eq, toU64, fromU64` |
| `SlotMapI64` | `class` | Fields: `values: [128]i64, versions: [128]u32, next_free: [128]u32, free_head: u32, count: usize` · Methods: `new, len, capacity, isEmpty, containsKey, insert, +4 more` |
| `SecondaryMapI64` | `class` | Fields: `values: [128]i64, versions: [128]u32, count: usize` · Methods: `new, len, containsKey, insert, get, remove, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `slmIsOccupiedVersion` | `(version: u32): bool` | Invokes `slmIsOccupiedVersion` and returns `bool`. |
| `slmAllocVersion` | `(cur_version: u32): u32` | Invokes `slmAllocVersion` and returns `u32`. |
| `slmFreeVersion` | `(cur_version: u32): u32` | Invokes `slmFreeVersion` and returns `u32`. |

## Minimal Example

```cp
import { slmIsOccupiedVersion } from "slotmap";

pub fn main() {
    let result = slmIsOccupiedVersion(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
