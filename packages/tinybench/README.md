[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tinybench

Zero-heap micro-benchmark statistics engine (mean, ops/sec, min/max, p50/p75/p95/p99, stddev, RME)

| Metadata | Value |
| --- | --- |
| Package | `tinybench` |
| Version | `3.2.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tinybench
```

```cp
import {
    BenchStats,
    tbnSqrt,
    tbnSortAsc,
    tbnPercentile,
    computeStats,
    speedupRatio,
} from "tinybench";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BenchStats` | `class` | Fields: `count: usize, total_ns: f64, mean_ns: f64, min_ns: f64, max_ns: f64, variance: f64, +7 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `tbnSqrt` | `(x: f64): f64` | Invokes `tbnSqrt` and returns `f64`. |
| `tbnSortAsc` | `(arr: []f64)` | Executes `tbnSortAsc` in-place. |
| `tbnPercentile` | `(sorted: &[]f64, p: f64): f64` | Invokes `tbnPercentile` and returns `f64`. |
| `computeStats` | `(samples_ns: &[]f64): BenchStats` | Invokes `computeStats` and returns `BenchStats`. |
| `speedupRatio` | `(baseline: BenchStats, candidate: BenchStats): f64` | Invokes `speedupRatio` and returns `f64`. |

## Minimal Example

```cp
import { tbnSqrt } from "tinybench";

pub fn main() {
    let result = tbnSqrt(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
