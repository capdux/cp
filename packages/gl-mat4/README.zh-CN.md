[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# gl-mat4

Zero-allocation 4x4 3D graphics transformation matrix library (translation, scale, rotation, perspective, orthographic, lookAt) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:gl-mat4` ([https://github.com/stackgl/gl-mat4](https://github.com/stackgl/gl-mat4)) / `crates.io:nalgebra-glm` ([https://github.com/dimforge/nalgebra](https://github.com/dimforge/nalgebra))（原库名 `gl-mat4` → CP 包名 `gl-mat4`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `gl-mat4` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add gl-mat4
```

```cp
import {
    Vec3D,
    Mat4,
    absF64,
    sqrtF64,
    reduceAngle,
    sinF64,
    cosF64,
    tanF64,
} from "gl-mat4";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Vec3D` | `class` | 字段: `x: f64, y: f64, z: f64` · 方法: `new, approxEq` |
| `Mat4` | `class` | 字段: `m: [16]f64` · 方法: `zero, identity, fromTranslation, fromScale, fromRotationX, fromRotationY, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `reduceAngle` | `(rad: f64): f64` | 调用 `reduceAngle` 并返回 `f64`。 |
| `sinF64` | `(rad: f64): f64` | 调用 `sinF64` 并返回 `f64`。 |
| `cosF64` | `(rad: f64): f64` | 调用 `cosF64` 并返回 `f64`。 |
| `tanF64` | `(rad: f64): f64` | 调用 `tanF64` 并返回 `f64`。 |
| `v3Sub` | `(a: Vec3D, b: Vec3D): Vec3D` | 调用 `v3Sub` 并返回 `Vec3D`。 |
| `v3Dot` | `(a: Vec3D, b: Vec3D): f64` | 调用 `v3Dot` 并返回 `f64`。 |
| `v3Cross` | `(a: Vec3D, b: Vec3D): Vec3D` | 调用 `v3Cross` 并返回 `Vec3D`。 |
| `v3Normalize` | `(v: Vec3D): Vec3D` | 调用 `v3Normalize` 并返回 `Vec3D`。 |
| `identity` | `(): Mat4` | 调用 `identity` 并返回 `Mat4`。 |
| `zero` | `(): Mat4` | 调用 `zero` 并返回 `Mat4`。 |
| `fromTranslation` | `(tx: f64, ty: f64, tz: f64): Mat4` | 调用 `fromTranslation` 并返回 `Mat4`。 |
| `fromScale` | `(sx: f64, sy: f64, sz: f64): Mat4` | 调用 `fromScale` 并返回 `Mat4`。 |
| `fromRotationX` | `(rad: f64): Mat4` | 调用 `fromRotationX` 并返回 `Mat4`。 |
| `fromRotationY` | `(rad: f64): Mat4` | 调用 `fromRotationY` 并返回 `Mat4`。 |
| `fromRotationZ` | `(rad: f64): Mat4` | 调用 `fromRotationZ` 并返回 `Mat4`。 |
| `orthographic` | `(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64): Mat4` | 调用 `orthographic` 并返回 `Mat4`。 |
| `perspective` | `(fov_y_rad: f64, aspect: f64, near: f64, far: f64): Mat4` | 调用 `perspective` 并返回 `Mat4`。 |
| `lookAt` | `(eye: Vec3D, target: Vec3D, up: Vec3D): Mat4` | 调用 `lookAt` 并返回 `Mat4`。 |
| `mul` | `(a: Mat4, b: Mat4): Mat4` | 调用 `mul` 并返回 `Mat4`。 |
| `transpose` | `(a: Mat4): Mat4` | 调用 `transpose` 并返回 `Mat4`。 |
| `transformPoint` | `(mat: Mat4, p: Vec3D): Vec3D` | 调用 `transformPoint` 并返回 `Vec3D`。 |
| `transformDir` | `(mat: Mat4, v: Vec3D): Vec3D` | 调用 `transformDir` 并返回 `Vec3D`。 |
| `approxEq` | `(a: Mat4, b: Mat4, eps: f64): bool` | 调用 `approxEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { absF64 } from "gl-mat4";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
