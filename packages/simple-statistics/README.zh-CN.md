[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# simple-statistics

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Welford online statistics, Chan parallel merge, descriptive slice stats, covariance, Pearson correlation, and linear-interpolated percentiles.

## 安装与导入

```bash
cap add simple-statistics
```

```cp
import { absF64 } from "simple-statistics";
```

## 核心导出 API

- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn sum(xs: &[]f64): f64;`
- `export fn mean(xs: &[]f64): f64;`
- `export fn variance(xs: &[]f64): f64;`
- `export fn stddev(xs: &[]f64): f64;`
- `export fn minSlice(xs: &[]f64): f64;`
- `export fn maxSlice(xs: &[]f64): f64;`
- `export fn covariance(xs: &[]f64, ys: &[]f64): f64;`
- `export fn pearsonR(xs: &[]f64, ys: &[]f64): f64;`
- `export fn percentileSorted(sorted: &[]f64, p: f64): f64;`
- `export fn medianSorted(sorted: &[]f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`simple-statistics@1.0.0`)，许可证：MIT
