[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# lru-slab

Zero-heap pre-allocated O(1) intrusive LRU slab cache matching crates.io #415 lru-slab (358M+ downloads) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:lru-slab` ([https://github.com/rustls/lru-slab](https://github.com/rustls/lru-slab))

| Metadata | Value |
| --- | --- |
| Package | `lru-slab` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add lru-slab
```

```cp
import { GetResult, EvictResult, SlabEntry, LruSlab } from "lru-slab";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GetResult` | `class` | Fields: `ok: bool, val: u64` |
| `EvictResult` | `class` | Fields: `idx: i32, key: u64, val: u64` |
| `SlabEntry` | `class` | Fields: `occupied: bool, key: u64, val: u64, prev: i32, next: i32` |
| `LruSlab` | `class` | Fields: `head: i32, tail: i32, free_head: i32, count: usize, capacity: usize` · Methods: `new, init, len, isEmpty, isFull, unlinkLru, +8 more` |

## Minimal Example

```cp
import { GetResult } from "lru-slab";

pub fn main() {
    // Refer to src/index.d.cp for GetResult fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
