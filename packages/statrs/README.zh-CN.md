[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# statrs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap descriptive statistics, probability distributions (Normal, Exponential, Poisson, Binomial), error function, and simple linear regression (clean-room port of Rust statrs)

## 安装与导入

```bash
cap add statrs
```

```cp
import { statAbs } from "statrs";
```

## 核心导出 API

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

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`statrs@1.0.0`)，许可证：MIT
