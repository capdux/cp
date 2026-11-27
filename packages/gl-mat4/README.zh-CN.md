[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# gl-mat4

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 4x4 3D graphics transformation matrix library (translation, scale, rotation, perspective, orthographic, lookAt) for CP

## 安装与导入

```bash
cap add gl-mat4
```

```cp
import { absF64 } from "gl-mat4";
```

## 核心导出 API

- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn reduceAngle(rad: f64): f64;`
- `export fn sinF64(rad: f64): f64;`
- `export fn cosF64(rad: f64): f64;`
- `export fn tanF64(rad: f64): f64;`
- `export fn v3Sub(a: Vec3D, b: Vec3D): Vec3D;`
- `export fn v3Dot(a: Vec3D, b: Vec3D): f64;`
- `export fn v3Cross(a: Vec3D, b: Vec3D): Vec3D;`
- `export fn v3Normalize(v: Vec3D): Vec3D;`
- `export fn identity(): Mat4;`
- `export fn zero(): Mat4;`
- `export fn fromTranslation(tx: f64, ty: f64, tz: f64): Mat4;`
- `export fn fromScale(sx: f64, sy: f64, sz: f64): Mat4;`
- `export fn fromRotationX(rad: f64): Mat4;`
- `export fn fromRotationY(rad: f64): Mat4;`
- `export fn fromRotationZ(rad: f64): Mat4;`
- `export fn orthographic(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64): Mat4;`
- `export fn perspective(fov_y_rad: f64, aspect: f64, near: f64, far: f64): Mat4;`
- `export fn lookAt(eye: Vec3D, target: Vec3D, up: Vec3D): Mat4;`
- `export fn mul(a: Mat4, b: Mat4): Mat4;`
- `export fn transpose(a: Mat4): Mat4;`
- `export fn transformPoint(mat: Mat4, p: Vec3D): Vec3D;`
- `export fn transformDir(mat: Mat4, v: Vec3D): Vec3D;`
- `export fn approxEq(a: Mat4, b: Mat4, eps: f64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`gl-mat4@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`nalgebra-glm@1.0.0`)，许可证：MIT
