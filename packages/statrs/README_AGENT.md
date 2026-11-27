# statrs@1.0.0 (Agent Quickref)

> Zero-heap descriptive statistics, probability distributions (Normal, Exponential, Poisson, Binomial), error function, and simple linear regression (clean-room port of Rust statrs) | License: MIT
> Import: `import { ... } from "statrs";` | Full types: `src/index.d.cp`

## Functions
- `export fn statAbs(x: f64): f64;`
- `export fn statSqrt(x: f64): f64;`
- `export fn statExp(x: f64): f64;`
- `export fn statGcdU64(a: u64, b: u64): u64;`
- `export fn mean(xs: []f64): f64?;`
- `export fn popVariance(xs: []f64): f64?;`
- `export fn variance(xs: []f64): f64?;`
- `export fn stdDev(xs: []f64): f64?;`
- `export fn covariance(xs: []f64, ys: []f64): f64?;`
- `export fn pearsonCorr(xs: []f64, ys: []f64): f64?;`
- `export fn erf(x: f64): f64;`
- `export fn normalPdf(x: f64, mu: f64, sigma: f64): f64;`
- `export fn normalCdf(x: f64, mu: f64, sigma: f64): f64;`
- `export fn exponentialPdf(x: f64, rate: f64): f64;`
- `export fn exponentialCdf(x: f64, rate: f64): f64;`
- `export fn poissonPmf(k: u32, lambda: f64): f64;`
- `export fn binomialCoeff(n: u32, k: u32): u64;`
- `export fn linearRegression(xs: []f64, ys: []f64): LinearFit;`
