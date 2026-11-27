[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# d3-array

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation array statistics, R-7 quantiles, median, variance, deviation, bisection search, and histogram binning (clean-room MIT implementation of npm d3-array, #345 / 153M wk)

## 安装与导入

```bash
cap add d3-array
```

```cp
import { sumF64 } from "d3-array";
```

## 核心导出 API

- `export fn sumF64(data: &[]f64): f64;`
- `export fn meanF64(data: &[]f64): f64?;`
- `export fn extentF64(data: &[]f64): ExtentF64;`
- `export fn varianceF64(data: &[]f64): f64?;`
- `export fn deviationF64(data: &[]f64): f64?;`
- `export fn quantileSorted(sorted: &[]f64, p: f64): f64?;`
- `export fn medianSorted(sorted: &[]f64): f64?;`
- `export fn bisectLeft(sorted: &[]f64, x: f64): usize;`
- `export fn bisectRight(sorted: &[]f64, x: f64): usize;`
- `export fn cumsumF64(data: &[]f64, out: []f64): usize;`
- `export fn binCounts(data: &[]f64, domain_min: f64, domain_max: f64, out_counts: []usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`d3-array@1.0.0`)，许可证：ISC
