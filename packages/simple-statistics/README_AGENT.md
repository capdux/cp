# simple-statistics — AI Agent Quick Ref

Zero-heap Welford online statistics, Chan parallel merge, descriptive slice stats, covariance, Pearson correlation, and linear-interpolated percentiles. (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class OnlineStats { count: u64, mean: f64, m2: f64, min_val: f64, /* +1 fields */ } // methods: new(...) +6
```

## API Signatures

```cp
fn absF64(x: f64): f64;
fn sqrtF64(x: f64): f64;
fn sum(xs: &[]f64): f64;
fn mean(xs: &[]f64): f64;
fn variance(xs: &[]f64): f64;
fn stddev(xs: &[]f64): f64;
fn minSlice(xs: &[]f64): f64;
fn maxSlice(xs: &[]f64): f64;
fn covariance(xs: &[]f64, ys: &[]f64): f64;
fn pearsonR(xs: &[]f64, ys: &[]f64): f64;
fn percentileSorted(sorted: &[]f64, p: f64): f64;
fn medianSorted(sorted: &[]f64): f64;
```

## Minimal Usage

```cp
import { absF64 } from "simple-statistics";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"simple-statistics"`; install via `cap add simple-statistics`.
