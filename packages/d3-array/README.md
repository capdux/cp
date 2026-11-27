[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# d3-array

Zero-allocation array statistics, R-7 quantiles, median, variance, deviation, bisection search, and histogram binning (clean-room MIT implementation of npm d3-array, #345 / 153M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-array` ([https://github.com/d3/d3-array](https://github.com/d3/d3-array)) | 许可：`ISC`

| Metadata | Value |
| --- | --- |
| Package | `d3-array` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add d3-array
```

```cp
import {
    ExtentF64,
    d3aIsNan,
    sumF64,
    meanF64,
    extentF64,
    varianceF64,
    deviationF64,
    quantileSorted,
} from "d3-array";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ExtentF64` | `class` | Fields: `valid: bool, min: f64, max: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `d3aIsNan` | `(v: f64): bool` | Invokes `d3aIsNan` and returns `bool`. |
| `sumF64` | `(data: &[]f64): f64` | Invokes `sumF64` and returns `f64`. |
| `meanF64` | `(data: &[]f64): f64?` | Invokes `meanF64` and returns `f64?`. |
| `extentF64` | `(data: &[]f64): ExtentF64` | Invokes `extentF64` and returns `ExtentF64`. |
| `varianceF64` | `(data: &[]f64): f64?` | Invokes `varianceF64` and returns `f64?`. |
| `deviationF64` | `(data: &[]f64): f64?` | Invokes `deviationF64` and returns `f64?`. |
| `quantileSorted` | `(sorted: &[]f64, p: f64): f64?` | Invokes `quantileSorted` and returns `f64?`. |
| `medianSorted` | `(sorted: &[]f64): f64?` | Invokes `medianSorted` and returns `f64?`. |
| `bisectLeft` | `(sorted: &[]f64, x: f64): usize` | Invokes `bisectLeft` and returns `usize`. |
| `bisectRight` | `(sorted: &[]f64, x: f64): usize` | Invokes `bisectRight` and returns `usize`. |
| `cumsumF64` | `(data: &[]f64, out: []f64): usize` | Invokes `cumsumF64` and returns `usize`. |
| `binCounts` | `(data: &[]f64, domain_min: f64, domain_max: f64, out_counts: []usize): usize` | Invokes `binCounts` and returns `usize`. |

## Minimal Example

```cp
import { d3aIsNan } from "d3-array";

pub fn main() {
    let result = d3aIsNan(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
