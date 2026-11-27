[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# yallist

Zero-allocation O(1) doubly linked list backed by a fixed 64-slot freelist pool (npm yallist v5 parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`yallist`](https://github.com/isaacs/yallist)（`ISC`）（包名 `yallist`）。

| Metadata | Value |
| --- | --- |
| Package | `yallist` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add yallist
```

```cp
import { YallistU64, yalValidHandle } from "yallist";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `YallistU64` | `class` | Fields: `values: [64]u64, prev: [64]i32, next: [64]i32, alive: [64]bool, free_stack: [64]i32, free_top: usize, +3 more` · Methods: `new, len, capacity, isEmpty, push, pop, +10 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `yalValidHandle` | `(handle: i32): bool` | Invokes `yalValidHandle` and returns `bool`. |

## Minimal Example

```cp
import { yalValidHandle } from "yallist";

pub fn main() {
    let result = yalValidHandle(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
