[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# average

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Welford streaming statistics, Chan parallel merge, OLS linear regression, and linear-interpolated percentiles for CP

## 安装与导入

```bash
cap add average
```

```cp
import { linearRegression } from "average";
```

## 核心导出 API

- `export fn linearRegression(xs: &[]f64, ys: &[]f64): LinearFit;`
- `export fn percentile(values: &[]f64, p: f64): f64?;`
- `export fn median(values: &[]f64): f64?;`
- `export fn iqr(values: &[]f64): f64?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`average@1.0.0`)，许可证：MIT
