[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# d3-interpolate

> 版本：`3.0.1` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation numeric, integer, RGB/HSL color, gamma-corrected, and uniform B-spline interpolators

## 安装与导入

```bash
cap add d3-interpolate
```

```cp
import { interpolateNumber } from "d3-interpolate";
```

## 核心导出 API

- `export fn interpolateNumber(a: f64, b: f64, t: f64): f64;`
- `export fn interpolateRound(a: i64, b: i64, t: f64): i64;`
- `export fn interpolateDiscrete(values: &[]f64, t: f64): f64;`
- `export fn interpolateRgb(c0: RgbColor, c1: RgbColor, t: f64): RgbColor;`
- `export fn interpolateHue(a: f64, b: f64, t: f64): f64;`
- `export fn interpolateHsl(c0: HslColor, c1: HslColor, t: f64): HslColor;`
- `export fn basis(t1: f64, v0: f64, v1: f64, v2: f64, v3: f64): f64;`
- `export fn interpolateBasis(values: &[]f64, t: f64): f64;`
- `export fn interpolatePiecewise(values: &[]f64, t: f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`d3-interpolate@3.0.1`)（https://github.com/d3/d3-interpolate），许可证：ISC
