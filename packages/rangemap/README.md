[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rangemap

Zero-allocation sorted disjoint half-open interval set [start, end) with automatic coalescing, subtraction/splitting, binary-search lookup, and span calculation (Rust rangemap / npm merge-ranges parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rangemap` ([https://github.com/jeffparsons/rangemap](https://github.com/jeffparsons/rangemap))（原库名 `rangemap` → CP 包名 `rangemap`）

| Metadata | Value |
| --- | --- |
| Package | `rangemap` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rangemap
```

```cp
import { Range, RangeSet } from "rangemap";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Range` | `class` | Fields: `start: i64, end: i64` |
| `RangeSet` | `class` | Fields: `ranges: [MAX_RANGES]Range, count: usize` · Methods: `init, len, isEmpty, clear, get, insert, +6 more` |

## Minimal Example

```cp
import { Range } from "rangemap";

pub fn main() {
    // Refer to src/index.d.cp for Range fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
