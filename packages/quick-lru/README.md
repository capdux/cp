[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# quick-lru

Zero-heap two-generation QuickLRU cache (QuickLruI64) with O(1) rotation eviction (clean-room port of npm quick-lru)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:quick-lru` ([https://github.com/sindresorhus/quick-lru](https://github.com/sindresorhus/quick-lru))

| Metadata | Value |
| --- | --- |
| Package | `quick-lru` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add quick-lru
```

```cp
import { QlruSlot, QuickLruI64, qlruStrEq } from "quick-lru";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `QlruSlot` | `class` | Fields: `occupied: bool, key: string, val: i64, hash: u64` |
| `QuickLruI64` | `class` | Fields: `cur: [32]QlruSlot, old: [32]QlruSlot, cur_len: usize, old_len: usize, max_size: usize` · Methods: `new, maxSize, size, findInCur, findInOld, rotateGenerations, +7 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `qlruStrEq` | `(a: string, b: string): bool` | Invokes `qlruStrEq` and returns `bool`. |

## Minimal Example

```cp
import { qlruStrEq } from "quick-lru";

pub fn main() {
    let result = qlruStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
