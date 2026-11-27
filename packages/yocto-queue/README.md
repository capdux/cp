[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# yocto-queue

Tiny O(1) FIFO queue data structure (clean-room zero-heap CP implementation of npm yocto-queue)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:yocto-queue` ([https://github.com/sindresorhus/yocto-queue](https://github.com/sindresorhus/yocto-queue))

| Metadata | Value |
| --- | --- |
| Package | `yocto-queue` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add yocto-queue
```

```cp
import { PopU64Result, PopI64Result, YoctoQueueU64, YoctoQueueI64 } from "yocto-queue";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PopU64Result` | `class` | Fields: `ok: bool, value: u64` |
| `PopI64Result` | `class` | Fields: `ok: bool, value: i64` |
| `YoctoQueueU64` | `class` | Fields: `buf: [256]u64, head: usize, tail: usize, count: usize` · Methods: `new, size, capacity, isEmpty, isFull, enqueue, +5 more` |
| `YoctoQueueI64` | `class` | Fields: `buf: [256]i64, head: usize, tail: usize, count: usize` · Methods: `new, size, isEmpty, isFull, enqueue, dequeue, +2 more` |

## Minimal Example

```cp
import { PopU64Result } from "yocto-queue";

pub fn main() {
    // Refer to src/index.d.cp for PopU64Result fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
