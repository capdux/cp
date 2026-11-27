[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sharded-slab

Zero-allocation O(1) generational slab allocator and ABA-safe handle pool (Rust slab + generational-arena parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sharded-slab` ([https://github.com/hawkw/sharded-slab](https://github.com/hawkw/sharded-slab))（原库名 `sharded-slab` → CP 包名 `sharded-slab`）

| Metadata | Value |
| --- | --- |
| Package | `sharded-slab` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sharded-slab
```

```cp
import { SlabKey, SlabI64, SlabU64, nextGeneration } from "sharded-slab";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SlabKey` | `class` | Fields: `index: u32, generation: u32` · Methods: `equals` |
| `SlabI64` | `class` | Fields: `slots: [SLAB_CAPACITY]i64, occupied: [SLAB_CAPACITY]bool, generations: [SLAB_CAPACITY]u32, next_free: [SLAB_CAPACITY]i32, free_head: i32, next_alloc: usize, +2 more` · Methods: `init, insert, contains, get, set, remove, +4 more` |
| `SlabU64` | `class` | Fields: `slots: [SLAB_CAPACITY]u64, occupied: [SLAB_CAPACITY]bool, generations: [SLAB_CAPACITY]u32, next_free: [SLAB_CAPACITY]i32, free_head: i32, next_alloc: usize, +2 more` · Methods: `init, insert, contains, get, set, remove, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nextGeneration` | `(gen: u32): u32` | Invokes `nextGeneration` and returns `u32`. |

## Minimal Example

```cp
import { nextGeneration } from "sharded-slab";

pub fn main() {
    let result = nextGeneration(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
