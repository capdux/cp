[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# simple-statistics

Zero-heap Welford online statistics, Chan parallel merge, descriptive slice stats, covariance, Pearson correlation, and linear-interpolated percentiles.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:simple-statistics` ([https://github.com/simple-statistics/simple-statistics](https://github.com/simple-statistics/simple-statistics))（原库名 `simple-statistics` → CP 包名 `simple-statistics`）

| Metadata | Value |
| --- | --- |
| Package | `simple-statistics` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add simple-statistics
```

```cp
import {
    OnlineStats,
    absF64,
    sqrtF64,
    sum,
    mean,
    variance,
    stddev,
    minSlice,
} from "simple-statistics";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OnlineStats` | `class` | Fields: `count: u64, mean: f64, m2: f64, min_val: f64, max_val: f64` · Methods: `new, push, merge, variance, sampleVariance, stddev, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `sum` | `(xs: &[]f64): f64` | Invokes `sum` and returns `f64`. |
| `mean` | `(xs: &[]f64): f64` | Invokes `mean` and returns `f64`. |
| `variance` | `(xs: &[]f64): f64` | Invokes `variance` and returns `f64`. |
| `stddev` | `(xs: &[]f64): f64` | Invokes `stddev` and returns `f64`. |
| `minSlice` | `(xs: &[]f64): f64` | Invokes `minSlice` and returns `f64`. |
| `maxSlice` | `(xs: &[]f64): f64` | Invokes `maxSlice` and returns `f64`. |
| `covariance` | `(xs: &[]f64, ys: &[]f64): f64` | Invokes `covariance` and returns `f64`. |
| `pearsonR` | `(xs: &[]f64, ys: &[]f64): f64` | Invokes `pearsonR` and returns `f64`. |
| `percentileSorted` | `(sorted: &[]f64, p: f64): f64` | Invokes `percentileSorted` and returns `f64`. |
| `medianSorted` | `(sorted: &[]f64): f64` | Invokes `medianSorted` and returns `f64`. |

## Minimal Example

```cp
import { absF64 } from "simple-statistics";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
