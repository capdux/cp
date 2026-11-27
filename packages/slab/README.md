[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# slab

Zero-heap pre-allocated O(1) slab allocator with intrusive free-list (clean-room port of Rust slab)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:slab` ([https://github.com/tokio-rs/slab](https://github.com/tokio-rs/slab))（原库名 `slab` → CP 包名 `slab`）

| Metadata | Value |
| --- | --- |
| Package | `slab` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add slab
```

```cp
import { SlabEntryI64, SlabI64 } from "slab";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SlabEntryI64` | `class` | Fields: `occupied: bool, val: i64, next_free: usize` |
| `SlabI64` | `class` | Fields: `entries: [128]SlabEntryI64, count: usize, next_free: usize, watermark: usize` · Methods: `new, len, capacity, isEmpty, isFull, vacantKey, +7 more` |

## Minimal Example

```cp
import { SlabEntryI64 } from "slab";

pub fn main() {
    // Refer to src/index.d.cp for SlabEntryI64 fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
