[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# priority-queue

Zero-allocation Indexed Binary Min-Heap with O(log N) dynamic changePriority and arbitrary remove (Rust priority-queue / heap-js parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:priority-queue` ([https://github.com/garro95/priority-queue](https://github.com/garro95/priority-queue))

| Metadata | Value |
| --- | --- |
| Package | `priority-queue` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add priority-queue
```

```cp
import { PqItem, IndexedMinHeap } from "priority-queue";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PqItem` | `class` | Fields: `id: u16, priority: i64, valid: bool` |
| `IndexedMinHeap` | `class` | Fields: `heap_ids: [MAX_ITEMS]u16, priorities: [MAX_ITEMS]i64, pos: [MAX_ITEMS]i16, size: usize` · Methods: `init, len, isEmpty, contains, priorityOf, peek, +9 more` |

## Minimal Example

```cp
import { PqItem } from "priority-queue";

pub fn main() {
    // Refer to src/index.d.cp for PqItem fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
