[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# average

Zero-allocation Welford streaming statistics, Chan parallel merge, OLS linear regression, and linear-interpolated percentiles for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:average` ([https://github.com/vks/average](https://github.com/vks/average))（原库名 `average` → CP 包名 `average`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `average` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add average
```

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

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RunningStats` | `class` | 字段: `n: u64, mean_val: f64, m2: f64, min_val: f64, max_val: f64, sum_val: f64` · 方法: `init, push, merge, count, mean, varianceSample, +6 项` |
| `LinearFit` | `class` | 字段: `valid: bool, slope: f64, intercept: f64, r_squared: f64, pearson_r: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `linearRegression` | `(xs: &[]f64, ys: &[]f64): LinearFit` | 调用 `linearRegression` 并返回 `LinearFit`。 |
| `copyAndSort` | `(values: &[]f64, buf: &mut [MAX_PERCENTILE_SAMPLES]f64): usize` | 调用 `copyAndSort` 并返回 `usize`。 |
| `percentileFromSorted` | `(sorted: &mut [MAX_PERCENTILE_SAMPLES]f64, n: usize, p: f64): f64` | 调用 `percentileFromSorted` 并返回 `f64`。 |
| `percentile` | `(values: &[]f64, p: f64): f64?` | 调用 `percentile` 并返回 `f64?`。 |
| `median` | `(values: &[]f64): f64?` | 调用 `median` 并返回 `f64?`。 |
| `iqr` | `(values: &[]f64): f64?` | 调用 `iqr` 并返回 `f64?`。 |

## 最小调用示例

```cp
import { sqrtF64 } from "average";

pub fn main() {
    let result = sqrtF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
