[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# statrs

Zero-heap descriptive statistics, probability distributions (Normal, Exponential, Poisson, Binomial), error function, and simple linear regression (clean-room port of Rust statrs)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:statrs` ([https://github.com/statrs-dev/statrs](https://github.com/statrs-dev/statrs))

| Metadata | Value |
| --- | --- |
| Package | `statrs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add statrs
```

```cp
import {
    LinearFit,
    statAbs,
    statSqrt,
    statExp,
    statGcdU64,
    mean,
    popVariance,
    variance,
} from "statrs";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LinearFit` | `class` | Fields: `slope: f64, intercept: f64, r_squared: f64, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `statAbs` | `(x: f64): f64` | Invokes `statAbs` and returns `f64`. |
| `statSqrt` | `(x: f64): f64` | Invokes `statSqrt` and returns `f64`. |
| `statExp` | `(x: f64): f64` | Invokes `statExp` and returns `f64`. |
| `statGcdU64` | `(a: u64, b: u64): u64` | Invokes `statGcdU64` and returns `u64`. |
| `mean` | `(xs: []f64): f64?` | Invokes `mean` and returns `f64?`. |
| `popVariance` | `(xs: []f64): f64?` | Invokes `popVariance` and returns `f64?`. |
| `variance` | `(xs: []f64): f64?` | Invokes `variance` and returns `f64?`. |
| `stdDev` | `(xs: []f64): f64?` | Invokes `stdDev` and returns `f64?`. |
| `covariance` | `(xs: []f64, ys: []f64): f64?` | Invokes `covariance` and returns `f64?`. |
| `pearsonCorr` | `(xs: []f64, ys: []f64): f64?` | Invokes `pearsonCorr` and returns `f64?`. |
| `erf` | `(x: f64): f64` | Invokes `erf` and returns `f64`. |
| `normalPdf` | `(x: f64, mu: f64, sigma: f64): f64` | Invokes `normalPdf` and returns `f64`. |
| `normalCdf` | `(x: f64, mu: f64, sigma: f64): f64` | Invokes `normalCdf` and returns `f64`. |
| `exponentialPdf` | `(x: f64, rate: f64): f64` | Invokes `exponentialPdf` and returns `f64`. |
| `exponentialCdf` | `(x: f64, rate: f64): f64` | Invokes `exponentialCdf` and returns `f64`. |
| `poissonPmf` | `(k: u32, lambda: f64): f64` | Invokes `poissonPmf` and returns `f64`. |
| `binomialCoeff` | `(n: u32, k: u32): u64` | Invokes `binomialCoeff` and returns `u64`. |
| `linearRegression` | `(xs: []f64, ys: []f64): LinearFit` | Invokes `linearRegression` and returns `LinearFit`. |

## Minimal Example

```cp
import { statAbs } from "statrs";

pub fn main() {
    let result = statAbs(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
