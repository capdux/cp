# average — AI Agent Quick Ref

Zero-allocation Welford streaming statistics, Chan parallel merge, OLS linear regression, and linear-interpolated percentiles for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class RunningStats { n: u64, mean_val: f64, m2: f64, min_val: f64, /* +2 fields */ } // methods: init(...) +11
class LinearFit { valid: bool, slope: f64, intercept: f64, r_squared: f64, /* +1 fields */ }
```

## API Signatures

```cp
fn sqrtF64(x: f64): f64;
fn linearRegression(xs: &[]f64, ys: &[]f64): LinearFit;
fn copyAndSort(values: &[]f64, buf: &mut [MAX_PERCENTILE_SAMPLES]f64): usize;
fn percentileFromSorted(sorted: &mut [MAX_PERCENTILE_SAMPLES]f64, n: usize, p: f64): f64;
fn percentile(values: &[]f64, p: f64): f64?;
fn median(values: &[]f64): f64?;
fn iqr(values: &[]f64): f64?;
```

## Minimal Usage

```cp
import { sqrtF64 } from "average";

pub fn main() {
    let result = sqrtF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"average"`; install via `cap add average`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
