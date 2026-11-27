[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ndarray

Zero-heap fixed-capacity 2D/N-D numerical array with stride indexing, matmul, transpose, dot, and reductions (crates.io ndarray equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ndarray` ([https://github.com/rust-ndarray/ndarray](https://github.com/rust-ndarray/ndarray))

| Metadata | Value |
| --- | --- |
| Package | `ndarray` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ndarray
```

```cp
import { Array2D, addArrays, matmul, dotProduct } from "ndarray";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Array2D` | `class` | Fields: `rows: usize, cols: usize, data: [64]f64` · Methods: `zeros, filled, eye, fromSlice, elemCount, get, +8 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `addArrays` | `(a: Array2D, b: Array2D): Array2D?` | Mutates or appends state via `addArrays`. |
| `matmul` | `(a: Array2D, b: Array2D): Array2D?` | Invokes `matmul` and returns `Array2D?`. |
| `dotProduct` | `(a: []f64, b: []f64): f64?` | Invokes `dotProduct` and returns `f64?`. |

## Minimal Example

```cp
import { addArrays } from "ndarray";

pub fn main() {
    let result = addArrays(/* a: Array2D */, /* b: Array2D */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
