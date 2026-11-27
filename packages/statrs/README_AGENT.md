# statrs — AI Agent Quick Ref

Zero-heap descriptive statistics, probability distributions (Normal, Exponential, Poisson, Binomial), error function, and simple linear regression (clean-room port of Rust statrs) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class LinearFit { slope: f64, intercept: f64, r_squared: f64, valid: bool }
```

## API Signatures

```cp
fn statAbs(x: f64): f64;
fn statSqrt(x: f64): f64;
fn statExp(x: f64): f64;
fn statGcdU64(a: u64, b: u64): u64;
fn mean(xs: []f64): f64?;
fn popVariance(xs: []f64): f64?;
fn variance(xs: []f64): f64?;
fn stdDev(xs: []f64): f64?;
fn covariance(xs: []f64, ys: []f64): f64?;
fn pearsonCorr(xs: []f64, ys: []f64): f64?;
fn erf(x: f64): f64;
fn normalPdf(x: f64, mu: f64, sigma: f64): f64;
fn normalCdf(x: f64, mu: f64, sigma: f64): f64;
fn exponentialPdf(x: f64, rate: f64): f64;
fn exponentialCdf(x: f64, rate: f64): f64;
fn poissonPmf(k: u32, lambda: f64): f64;
fn binomialCoeff(n: u32, k: u32): u64;
fn linearRegression(xs: []f64, ys: []f64): LinearFit;
```

## Minimal Usage

```cp
import { statAbs } from "statrs";

pub fn main() {
    let result = statAbs(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"statrs"`; install via `cap add statrs`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
