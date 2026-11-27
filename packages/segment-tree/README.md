[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# segment-tree

Zero-allocation iterative 2*N Segment Tree with O(log N) rangeSum, rangeMin, rangeMax, and point updates (Rust segment-tree parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:segment-tree` ([https://github.com/Darksonn/segment-tree](https://github.com/Darksonn/segment-tree))

| Metadata | Value |
| --- | --- |
| Package | `segment-tree` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add segment-tree
```

```cp
import { RangeStats, SegmentTree, minI64, maxI64 } from "segment-tree";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RangeStats` | `class` | Fields: `sum: i64, min_val: i64, max_val: i64, valid: bool` |
| `SegmentTree` | `class` | Fields: `size: usize, sums: [TREE_CAP]i64, mins: [TREE_CAP]i64, maxs: [TREE_CAP]i64` · Methods: `init, fromSlice, len, get, set, add, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `minI64` | `(a: i64, b: i64): i64` | Invokes `minI64` and returns `i64`. |
| `maxI64` | `(a: i64, b: i64): i64` | Invokes `maxI64` and returns `i64`. |

## Minimal Example

```cp
import { minI64 } from "segment-tree";

pub fn main() {
    let result = minI64(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
