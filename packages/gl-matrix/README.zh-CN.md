[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# gl-matrix

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 3D vector and spatial geometry library for CP

## 安装与导入

```bash
cap add gl-matrix
```

```cp
import { sqrtF64 } from "gl-matrix";
```

## 核心导出 API

- `export fn sqrtF64(x: f64): f64;`
- `export fn absF64(x: f64): f64;`
- `export fn minF64(a: f64, b: f64): f64;`
- `export fn maxF64(a: f64, b: f64): f64;`
- `export fn new(x: f64, y: f64, z: f64): Vec3;`
- `export fn zero(): Vec3;`
- `export fn splat(v: f64): Vec3;`
- `export fn unitX(): Vec3;`
- `export fn unitY(): Vec3;`
- `export fn unitZ(): Vec3;`
- `export fn add(a: Vec3, b: Vec3): Vec3;`
- `export fn sub(a: Vec3, b: Vec3): Vec3;`
- `export fn mul(a: Vec3, b: Vec3): Vec3;`
- `export fn scale(v: Vec3, s: f64): Vec3;`
- `export fn neg(v: Vec3): Vec3;`
- `export fn dot(a: Vec3, b: Vec3): f64;`
- `export fn cross(a: Vec3, b: Vec3): Vec3;`
- `export fn lengthSq(v: Vec3): f64;`
- `export fn length(v: Vec3): f64;`
- `export fn distance(a: Vec3, b: Vec3): f64;`
- `export fn normalize(v: Vec3): Vec3;`
- `export fn lerp(a: Vec3, b: Vec3, t: f64): Vec3;`
- `export fn reflect(v: Vec3, normal: Vec3): Vec3;`
- `export fn project(v: Vec3, onto: Vec3): Vec3;`
- `export fn minVec(a: Vec3, b: Vec3): Vec3;`
- `export fn maxVec(a: Vec3, b: Vec3): Vec3;`
- `export fn clampLength(v: Vec3, max_len: f64): Vec3;`
- `export fn approxEq(a: Vec3, b: Vec3, eps: f64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`gl-matrix@1.0.0`)，许可证：MIT
