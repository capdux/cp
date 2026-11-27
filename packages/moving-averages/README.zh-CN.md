[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# moving-averages

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Exponential Moving Average (EMA), Simple Moving Average (SMA), and Weighted Moving Average (WMA) in pure CP

## 安装与导入

```bash
cap add moving-averages
```

```cp
import { smaSlice } from "moving-averages";
```

## 核心导出 API

- `export fn smaSlice(xs: []f64): f64;`
- `export fn wmaSlice(xs: []f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`moving-averages@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`simple_moving_average@1.0.0`)，许可证：MIT
