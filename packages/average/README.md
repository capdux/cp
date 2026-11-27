[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# average

Zero-allocation Welford streaming statistics, Chan parallel merge, OLS linear regression, and linear-interpolated percentiles for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:average` ([https://github.com/vks/average](https://github.com/vks/average))（原库名 `average` → CP 包名 `average`）

| Metadata | Value |
| --- | --- |
| Package | `average` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add average
```

```cp
import {
    RunningStats,
    LinearFit,
    sqrtF64,
    linearRegression,
    copyAndSort,
    percentileFromSorted,
    percentile,
    median,
} from "average";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RunningStats` | `class` | Fields: `n: u64, mean_val: f64, m2: f64, min_val: f64, max_val: f64, sum_val: f64` · Methods: `init, push, merge, count, mean, varianceSample, +6 more` |
| `LinearFit` | `class` | Fields: `valid: bool, slope: f64, intercept: f64, r_squared: f64, pearson_r: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `linearRegression` | `(xs: &[]f64, ys: &[]f64): LinearFit` | Invokes `linearRegression` and returns `LinearFit`. |
| `copyAndSort` | `(values: &[]f64, buf: &mut [MAX_PERCENTILE_SAMPLES]f64): usize` | Invokes `copyAndSort` and returns `usize`. |
| `percentileFromSorted` | `(sorted: &mut [MAX_PERCENTILE_SAMPLES]f64, n: usize, p: f64): f64` | Invokes `percentileFromSorted` and returns `f64`. |
| `percentile` | `(values: &[]f64, p: f64): f64?` | Invokes `percentile` and returns `f64?`. |
| `median` | `(values: &[]f64): f64?` | Invokes `median` and returns `f64?`. |
| `iqr` | `(values: &[]f64): f64?` | Invokes `iqr` and returns `f64?`. |

## Minimal Example

```cp
import { sqrtF64 } from "average";

pub fn main() {
    let result = sqrtF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
