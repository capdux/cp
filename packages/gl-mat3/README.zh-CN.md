[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# gl-mat3

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 3x3 row-major matrix and 2D affine transform library with built-in pure-CP sin/cos for CP.

## 安装与导入

```bash
cap add gl-mat3
```

```cp
import { zero } from "gl-mat3";
```

## 核心导出 API

- `export fn zero(): Mat3;`
- `export fn identity(): Mat3;`
- `export fn fromTranslation(tx: f64, ty: f64): Mat3;`
- `export fn fromScale(sx: f64, sy: f64): Mat3;`
- `export fn fromRotation(radians: f64): Mat3;`
- `export fn mul(a: Mat3, b: Mat3): Mat3;`
- `export fn transpose(a: Mat3): Mat3;`
- `export fn determinant(a: Mat3): f64;`
- `export fn isInvertible(a: Mat3): bool;`
- `export fn inverse(a: Mat3): Mat3;`
- `export fn transformPoint(mat: Mat3, p: Vec2D): Vec2D;`
- `export fn transformDir(mat: Mat3, v: Vec2D): Vec2D;`
- `export fn approxEq(a: Mat3, b: Mat3, eps: f64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`gl-mat3@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`cgmath@1.0.0`)，许可证：MIT
