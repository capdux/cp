# d3-array — AI Agent Quick Ref

Zero-allocation array statistics, R-7 quantiles, median, variance, deviation, bisection search, and histogram binning (clean-room MIT implementation of npm d3-array, #345 / 153M wk) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class ExtentF64 { valid: bool, min: f64, max: f64 }
```

## API Signatures

```cp
fn d3aIsNan(v: f64): bool;
fn sumF64(data: &[]f64): f64;
fn meanF64(data: &[]f64): f64?;
fn extentF64(data: &[]f64): ExtentF64;
fn varianceF64(data: &[]f64): f64?;
fn deviationF64(data: &[]f64): f64?;
fn quantileSorted(sorted: &[]f64, p: f64): f64?;
fn medianSorted(sorted: &[]f64): f64?;
fn bisectLeft(sorted: &[]f64, x: f64): usize;
fn bisectRight(sorted: &[]f64, x: f64): usize;
fn cumsumF64(data: &[]f64, out: []f64): usize;
fn binCounts(data: &[]f64, domain_min: f64, domain_max: f64, out_counts: []usize): usize;
```

## Minimal Usage

```cp
import { d3aIsNan } from "d3-array";

pub fn main() {
    let result = d3aIsNan(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"d3-array"`; install via `cap add d3-array`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
