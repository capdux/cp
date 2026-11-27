[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# statrs

Zero-heap descriptive statistics, probability distributions (Normal, Exponential, Poisson, Binomial), error function, and simple linear regression (clean-room port of Rust statrs)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:statrs` ([https://github.com/statrs-dev/statrs](https://github.com/statrs-dev/statrs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `statrs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

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

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LinearFit` | `class` | 字段: `slope: f64, intercept: f64, r_squared: f64, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `statAbs` | `(x: f64): f64` | 调用 `statAbs` 并返回 `f64`。 |
| `statSqrt` | `(x: f64): f64` | 调用 `statSqrt` 并返回 `f64`。 |
| `statExp` | `(x: f64): f64` | 调用 `statExp` 并返回 `f64`。 |
| `statGcdU64` | `(a: u64, b: u64): u64` | 调用 `statGcdU64` 并返回 `u64`。 |
| `mean` | `(xs: []f64): f64?` | 调用 `mean` 并返回 `f64?`。 |
| `popVariance` | `(xs: []f64): f64?` | 调用 `popVariance` 并返回 `f64?`。 |
| `variance` | `(xs: []f64): f64?` | 调用 `variance` 并返回 `f64?`。 |
| `stdDev` | `(xs: []f64): f64?` | 调用 `stdDev` 并返回 `f64?`。 |
| `covariance` | `(xs: []f64, ys: []f64): f64?` | 调用 `covariance` 并返回 `f64?`。 |
| `pearsonCorr` | `(xs: []f64, ys: []f64): f64?` | 调用 `pearsonCorr` 并返回 `f64?`。 |
| `erf` | `(x: f64): f64` | 调用 `erf` 并返回 `f64`。 |
| `normalPdf` | `(x: f64, mu: f64, sigma: f64): f64` | 调用 `normalPdf` 并返回 `f64`。 |
| `normalCdf` | `(x: f64, mu: f64, sigma: f64): f64` | 调用 `normalCdf` 并返回 `f64`。 |
| `exponentialPdf` | `(x: f64, rate: f64): f64` | 调用 `exponentialPdf` 并返回 `f64`。 |
| `exponentialCdf` | `(x: f64, rate: f64): f64` | 调用 `exponentialCdf` 并返回 `f64`。 |
| `poissonPmf` | `(k: u32, lambda: f64): f64` | 调用 `poissonPmf` 并返回 `f64`。 |
| `binomialCoeff` | `(n: u32, k: u32): u64` | 调用 `binomialCoeff` 并返回 `u64`。 |
| `linearRegression` | `(xs: []f64, ys: []f64): LinearFit` | 调用 `linearRegression` 并返回 `LinearFit`。 |

## 最小调用示例

```cp
import { statAbs } from "statrs";

pub fn main() {
    let result = statAbs(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
