[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tinybench

Zero-heap micro-benchmark statistics engine (mean, ops/sec, min/max, p50/p75/p95/p99, stddev, RME)

| 元信息 | 值 |
| --- | --- |
| 包名 | `tinybench` |
| 版本 | `3.2.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tinybench
```

```cp
import {
    BenchStats,
    tbnSqrt,
    tbnSortAsc,
    tbnPercentile,
    computeStats,
    speedupRatio,
} from "tinybench";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BenchStats` | `class` | 字段: `count: usize, total_ns: f64, mean_ns: f64, min_ns: f64, max_ns: f64, variance: f64, +7 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `tbnSqrt` | `(x: f64): f64` | 调用 `tbnSqrt` 并返回 `f64`。 |
| `tbnSortAsc` | `(arr: []f64)` | 就地执行 `tbnSortAsc` 操作。 |
| `tbnPercentile` | `(sorted: &[]f64, p: f64): f64` | 调用 `tbnPercentile` 并返回 `f64`。 |
| `computeStats` | `(samples_ns: &[]f64): BenchStats` | 调用 `computeStats` 并返回 `BenchStats`。 |
| `speedupRatio` | `(baseline: BenchStats, candidate: BenchStats): f64` | 调用 `speedupRatio` 并返回 `f64`。 |

## 最小调用示例

```cp
import { tbnSqrt } from "tinybench";

pub fn main() {
    let result = tbnSqrt(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
