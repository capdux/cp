[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# float-cmp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap IEEE-754 floating-point approximate equality and ordering with dual epsilon/ULPs margins and ratio comparisons (clean-room port of Rust float-cmp)

## 安装与导入

```bash
cap add float-cmp
```

```cp
import { fcmpIsNanF64 } from "float-cmp";
```

## 核心导出 API

- `export fn fcmpIsNanF64(x: f64): bool;`
- `export fn fcmpIsNanF32(x: f32): bool;`
- `export fn approxEqF64(a: f64, b: f64, margin: F64Margin): bool;`
- `export fn approxEqF32(a: f32, b: f32, margin: F32Margin): bool;`
- `export fn approxEqRatioF64(a: f64, b: f64, ratio: f64): bool;`
- `export fn approxOrdF64(a: f64, b: f64, margin: F64Margin): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`float-cmp@1.0.0`)，许可证：MIT
