[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ringbuf

Zero-heap O(1) circular ring buffers (RingBufU8 & RingBufI64) with slice batching and overwrite mode (clean-room port of Rust ringbuf)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ringbuf` ([https://github.com/agerasev/ringbuf](https://github.com/agerasev/ringbuf))

| Metadata | Value |
| --- | --- |
| Package | `ringbuf` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ringbuf
```

```cp
import { RingBufU8, RingBufI64 } from "ringbuf";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RingBufU8` | `class` | Fields: `buf: [256]u8, head: usize, tail: usize, count: usize` · Methods: `new, len, capacity, freeLen, isEmpty, isFull, +7 more` |
| `RingBufI64` | `class` | Fields: `buf: [64]i64, head: usize, tail: usize, count: usize` · Methods: `new, len, capacity, isEmpty, isFull, push, +4 more` |

## Minimal Example

```cp
import { RingBufU8 } from "ringbuf";

pub fn main() {
    // Refer to src/index.d.cp for RingBufU8 fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
