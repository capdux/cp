[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fenwick

Zero-allocation Binary Indexed Tree (Fenwick Tree) with O(log N) prefix/range sums and binary-lifting lowerBound for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fenwick` ([https://github.com/pwoolcoc/fenwick](https://github.com/pwoolcoc/fenwick))

| Metadata | Value |
| --- | --- |
| Package | `fenwick` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fenwick
```

```cp
import { FenwickTree, lowbit } from "fenwick";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FenwickTree` | `class` | Fields: `tree: [TREE_LEN]i64, length: usize` · Methods: `init, fromSlice, len, add, prefixSum, rangeSum, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lowbit` | `(x: usize): usize` | Invokes `lowbit` and returns `usize`. |

## Minimal Example

```cp
import { lowbit } from "fenwick";

pub fn main() {
    let result = lowbit(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
