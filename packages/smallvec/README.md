[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# smallvec

Zero-heap inline stack vector with push/pop/insert/remove/swapRemove/dedup (clean-room CP implementation of crates.io smallvec)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:smallvec` ([https://github.com/servo/rust-smallvec](https://github.com/servo/rust-smallvec))

| Metadata | Value |
| --- | --- |
| Package | `smallvec` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add smallvec
```

```cp
import { SmallPopU64, SmallVecU64, SmallVecU8 } from "smallvec";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SmallPopU64` | `class` | Fields: `ok: bool, value: u64` |
| `SmallVecU64` | `class` | Fields: `data: [64]u64, used: usize` · Methods: `new, len, capacity, isEmpty, isFull, clear, +12 more` |
| `SmallVecU8` | `class` | Fields: `data: [256]u8, used: usize` · Methods: `new, len, capacity, isEmpty, clear, push, +2 more` |

## Minimal Example

```cp
import { SmallPopU64 } from "smallvec";

pub fn main() {
    // Refer to src/index.d.cp for SmallPopU64 fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
