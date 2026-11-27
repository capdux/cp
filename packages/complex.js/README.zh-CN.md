[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# complex.js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation complex number arithmetic, polar form, exponential, integer powers, and principal square root for CP

## 安装与导入

```bash
cap add complex.js
```

```cp
import { absF64 } from "complex.js";
```

## 核心导出 API

- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn reduceAngle(rad: f64): f64;`
- `export fn sinF64(rad: f64): f64;`
- `export fn cosF64(rad: f64): f64;`
- `export fn expF64(x: f64): f64;`
- `export fn atanSmall(x: f64): f64;`
- `export fn atanF64(x: f64): f64;`
- `export fn atan2F64(y: f64, x: f64): f64;`
- `export fn new(re: f64, im: f64): Complex;`
- `export fn zero(): Complex;`
- `export fn one(): Complex;`
- `export fn iUnit(): Complex;`
- `export fn fromPolar(r: f64, theta: f64): Complex;`
- `export fn add(a: Complex, b: Complex): Complex;`
- `export fn sub(a: Complex, b: Complex): Complex;`
- `export fn mul(a: Complex, b: Complex): Complex;`
- `export fn div(a: Complex, b: Complex): Complex;`
- `export fn conj(z: Complex): Complex;`
- `export fn neg(z: Complex): Complex;`
- `export fn scale(z: Complex, s: f64): Complex;`
- `export fn normSq(z: Complex): f64;`
- `export fn norm(z: Complex): f64;`
- `export fn arg(z: Complex): f64;`
- `export fn exp(z: Complex): Complex;`
- `export fn powInt(z: Complex, n: i32): Complex;`
- `export fn sqrt(z: Complex): Complex;`
- `export fn approxEq(a: Complex, b: Complex, eps: f64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`complex.js@1.0.0`)，许可证：MIT
