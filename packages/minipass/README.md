[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# minipass

Zero-allocation fixed-capacity ring-buffer byte stream with synchronous backpressure and piping for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:minipass` ([https://github.com/isaacs/minipass](https://github.com/isaacs/minipass))

| Metadata | Value |
| --- | --- |
| Package | `minipass` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add minipass
```

```cp
import { MinipassStream, mpassMin } from "minipass";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MinipassStream` | `class` | Fields: `buf: [4096]u8, head: usize, len: usize, flowing: bool, ended: bool, destroyed: bool` · Methods: `new, bufferedBytes, freeSpace, isPaused, isEnded, isDrained, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mpassMin` | `(a: usize, b: usize): usize` | Invokes `mpassMin` and returns `usize`. |

## Minimal Example

```cp
import { mpassMin } from "minipass";

pub fn main() {
    let result = mpassMin(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
