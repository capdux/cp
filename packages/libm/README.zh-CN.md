[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# libm

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation IEEE 754 f64 math library (libm parity)

## 安装与导入

```bash
cap add libm
```

```cp
import { fabs } from "libm";
```

## 核心导出 API

- `export fn fabs(x: f64): f64;`
- `export fn trunc(x: f64): f64;`
- `export fn floor(x: f64): f64;`
- `export fn ceil(x: f64): f64;`
- `export fn round(x: f64): f64;`
- `export fn fmod(x: f64, y: f64): f64;`
- `export fn copysign(x: f64, y: f64): f64;`
- `export fn fmin(x: f64, y: f64): f64;`
- `export fn fmax(x: f64, y: f64): f64;`
- `export fn sqrt(x: f64): f64;`
- `export fn cbrt(x: f64): f64;`
- `export fn hypot(x: f64, y: f64): f64;`
- `export fn exp(x: f64): f64;`
- `export fn ln(x: f64): f64;`
- `export fn log2(x: f64): f64;`
- `export fn log10(x: f64): f64;`
- `export fn pow(x: f64, y: f64): f64;`
- `export fn sin(x: f64): f64;`
- `export fn cos(x: f64): f64;`
- `export fn tan(x: f64): f64;`
- `export fn atan(x: f64): f64;`
- `export fn atan2(y: f64, x: f64): f64;`
- `export fn asin(x: f64): f64;`
- `export fn acos(x: f64): f64;`
- `export fn sinh(x: f64): f64;`
- `export fn cosh(x: f64): f64;`
- `export fn tanh(x: f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`libm@1.0.0`)，许可证：MIT
