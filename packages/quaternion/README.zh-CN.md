[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# quaternion

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 quaternion

## 安装与导入

```bash
cap add quaternion
```

```cp
import { absF64 } from "quaternion";
```

## 核心导出 API

- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn wrapAngle(x: f64): f64;`
- `export fn sinF64(x: f64): f64;`
- `export fn cosF64(x: f64): f64;`
- `export fn atanF64(x: f64): f64;`
- `export fn acosF64(x: f64): f64;`
- `export fn new(x: f64, y: f64, z: f64, w: f64): Quat;`
- `export fn identity(): Quat;`
- `export fn fromAxisAngle(ax: f64, ay: f64, az: f64, radians: f64): Quat;`
- `export fn fromEulerXYZ(rx: f64, ry: f64, rz: f64): Quat;`
- `export fn mul(a: Quat, b: Quat): Quat;`
- `export fn conjugate(q: Quat): Quat;`
- `export fn dot(a: Quat, b: Quat): f64;`
- `export fn length(q: Quat): f64;`
- `export fn inverse(q: Quat): Quat;`
- `export fn normalize(q: Quat): Quat;`
- `export fn nlerp(a: Quat, b: Quat, t: f64): Quat;`
- `export fn slerp(a: Quat, b: Quat, t: f64): Quat;`
- `export fn rotatePoint(q: Quat, p: Point3): Point3;`
- `export fn approxEq(a: Quat, b: Quat, eps: f64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`quaternion@1.0.0`)，许可证：MIT
