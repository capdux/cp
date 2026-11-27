[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# statrs

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:statrs` ([https://github.com/statrs-dev/statrs](https://github.com/statrs-dev/statrs))

Clean-room MIT, zero-heap descriptive statistics, probability distributions, and linear regression library for CP — modeled on crates.io `statrs` v0.17.

## Features

- **Zero heap allocation & pure CP**: self-contained `exp`, `sqrt`, and `erf` (Abramowitz & Stegun 7.1.26, error `< 1.5e-7`) with zero external dependencies.
- **Descriptive Statistics**: `mean`, `variance` (sample `n-1`), `popVariance` (population `n`), `stdDev`, `covariance`, and `pearsonCorr`.
- **Continuous & Discrete Distributions**: `normalPdf`, `normalCdf`, `exponentialPdf`, `exponentialCdf`, `poissonPmf`, and overflow-safe `binomialCoeff`.
- **Linear Regression**: `linearRegression` returning `LinearFit { slope, intercept, r_squared, valid }`.

## API

### Types
- `LinearFit { slope: f64, intercept: f64, r_squared: f64, valid: bool }`

### Functions
- `mean(xs: []f64): f64?`
- `variance(xs: []f64): f64?`
- `popVariance(xs: []f64): f64?`
- `stdDev(xs: []f64): f64?`
- `covariance(xs: []f64, ys: []f64): f64?`
- `pearsonCorr(xs: []f64, ys: []f64): f64?`
- `erf(x: f64): f64`
- `normalPdf(x: f64, mu: f64, sigma: f64): f64`
- `normalCdf(x: f64, mu: f64, sigma: f64): f64`
- `exponentialPdf(x: f64, rate: f64): f64`
- `exponentialCdf(x: f64, rate: f64): f64`
- `poissonPmf(k: u32, lambda: f64): f64`
- `binomialCoeff(n: u32, k: u32): u64`
- `linearRegression(xs: []f64, ys: []f64): LinearFit`

## License

MIT
