[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# simple-statistics

Zero-heap Welford online statistics, Chan parallel merge, descriptive slice stats, covariance, Pearson correlation, and linear-interpolated percentiles.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:simple-statistics` ([https://github.com/simple-statistics/simple-statistics](https://github.com/simple-statistics/simple-statistics))（原库名 `simple-statistics` → CP 包名 `simple-statistics`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `simple-statistics` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add simple-statistics
```

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

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OnlineStats` | `class` | 字段: `count: u64, mean: f64, m2: f64, min_val: f64, max_val: f64` · 方法: `new, push, merge, variance, sampleVariance, stddev, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `sum` | `(xs: &[]f64): f64` | 调用 `sum` 并返回 `f64`。 |
| `mean` | `(xs: &[]f64): f64` | 调用 `mean` 并返回 `f64`。 |
| `variance` | `(xs: &[]f64): f64` | 调用 `variance` 并返回 `f64`。 |
| `stddev` | `(xs: &[]f64): f64` | 调用 `stddev` 并返回 `f64`。 |
| `minSlice` | `(xs: &[]f64): f64` | 调用 `minSlice` 并返回 `f64`。 |
| `maxSlice` | `(xs: &[]f64): f64` | 调用 `maxSlice` 并返回 `f64`。 |
| `covariance` | `(xs: &[]f64, ys: &[]f64): f64` | 调用 `covariance` 并返回 `f64`。 |
| `pearsonR` | `(xs: &[]f64, ys: &[]f64): f64` | 调用 `pearsonR` 并返回 `f64`。 |
| `percentileSorted` | `(sorted: &[]f64, p: f64): f64` | 调用 `percentileSorted` 并返回 `f64`。 |
| `medianSorted` | `(sorted: &[]f64): f64` | 调用 `medianSorted` 并返回 `f64`。 |

## 最小调用示例

```cp
import { absF64 } from "simple-statistics";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
