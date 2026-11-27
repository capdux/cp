[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# d3-scale

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation D3 quantitative scales (LinearScale, PowScale, SqrtScale, LogScale, QuantizeScale) in pure CP

## 安装与导入

```bash
cap add d3-scale
```

```cp
import { d3scClamp01 } from "d3-scale";
```

## 核心导出 API

- `export fn d3scClamp01(t: f64): f64;`
- `export fn d3scLerp(a: f64, b: f64, t: f64): f64;`
- `export fn d3scSignedPow(x: f64, exp: f64): f64;`
- `export fn d3scTickIncrement(start: f64, stop: f64, count: usize): f64;`
- `export fn linearScale(d0: f64, d1: f64, r0: f64, r1: f64): LinearScale;`
- `export fn powScale(d0: f64, d1: f64, r0: f64, r1: f64, exponent: f64): PowScale;`
- `export fn sqrtScale(d0: f64, d1: f64, r0: f64, r1: f64): SqrtScale;`
- `export fn logScale(d0: f64, d1: f64, r0: f64, r1: f64, base: f64): LogScale;`
- `export fn quantizeScale(d0: f64, d1: f64, buckets: usize): QuantizeScale;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`d3-scale@1.0.0`)，许可证：ISC
