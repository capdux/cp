[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# crossbeam-queue

Zero-heap bounded ring queue (RingQueueU64) and wrap-around byte ring buffer (ByteRing) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crossbeam-queue` ([https://github.com/crossbeam-rs/crossbeam](https://github.com/crossbeam-rs/crossbeam)) / `npm:denque` ([https://github.com/invertase/denque](https://github.com/invertase/denque))（原库名 `crossbeam-queue` → CP 包名 `crossbeam-queue`）

| Metadata | Value |
| --- | --- |
| Package | `crossbeam-queue` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add crossbeam-queue
```

```cp
import { RingQueueU64, ByteRing } from "crossbeam-queue";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RingQueueU64` | `class` | Fields: `head: usize, tail: usize, count: usize, capacity: usize` · Methods: `new, len, isEmpty, isFull, remaining, clear, +4 more` |
| `ByteRing` | `class` | Fields: `head: usize, tail: usize, count: usize, capacity: usize` · Methods: `new, len, isEmpty, isFull, remaining, clear, +2 more` |

## Minimal Example

```cp
import { RingQueueU64 } from "crossbeam-queue";

pub fn main() {
    // Refer to src/index.d.cp for RingQueueU64 fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
