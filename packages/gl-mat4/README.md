[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# gl-mat4

Zero-allocation 4x4 3D graphics transformation matrix library (translation, scale, rotation, perspective, orthographic, lookAt) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:gl-mat4` ([https://github.com/stackgl/gl-mat4](https://github.com/stackgl/gl-mat4)) / `crates.io:nalgebra-glm` ([https://github.com/dimforge/nalgebra](https://github.com/dimforge/nalgebra))（原库名 `gl-mat4` → CP 包名 `gl-mat4`）

| Metadata | Value |
| --- | --- |
| Package | `gl-mat4` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Vec3D` | `class` | Fields: `x: f64, y: f64, z: f64` · Methods: `new, approxEq` |
| `Mat4` | `class` | Fields: `m: [16]f64` · Methods: `zero, identity, fromTranslation, fromScale, fromRotationX, fromRotationY, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `reduceAngle` | `(rad: f64): f64` | Invokes `reduceAngle` and returns `f64`. |
| `sinF64` | `(rad: f64): f64` | Invokes `sinF64` and returns `f64`. |
| `cosF64` | `(rad: f64): f64` | Invokes `cosF64` and returns `f64`. |
| `tanF64` | `(rad: f64): f64` | Invokes `tanF64` and returns `f64`. |
| `v3Sub` | `(a: Vec3D, b: Vec3D): Vec3D` | Invokes `v3Sub` and returns `Vec3D`. |
| `v3Dot` | `(a: Vec3D, b: Vec3D): f64` | Invokes `v3Dot` and returns `f64`. |
| `v3Cross` | `(a: Vec3D, b: Vec3D): Vec3D` | Invokes `v3Cross` and returns `Vec3D`. |
| `v3Normalize` | `(v: Vec3D): Vec3D` | Invokes `v3Normalize` and returns `Vec3D`. |
| `identity` | `(): Mat4` | Invokes `identity` and returns `Mat4`. |
| `zero` | `(): Mat4` | Invokes `zero` and returns `Mat4`. |
| `fromTranslation` | `(tx: f64, ty: f64, tz: f64): Mat4` | Invokes `fromTranslation` and returns `Mat4`. |
| `fromScale` | `(sx: f64, sy: f64, sz: f64): Mat4` | Invokes `fromScale` and returns `Mat4`. |
| `fromRotationX` | `(rad: f64): Mat4` | Invokes `fromRotationX` and returns `Mat4`. |
| `fromRotationY` | `(rad: f64): Mat4` | Invokes `fromRotationY` and returns `Mat4`. |
| `fromRotationZ` | `(rad: f64): Mat4` | Invokes `fromRotationZ` and returns `Mat4`. |
| `orthographic` | `(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64): Mat4` | Invokes `orthographic` and returns `Mat4`. |
| `perspective` | `(fov_y_rad: f64, aspect: f64, near: f64, far: f64): Mat4` | Invokes `perspective` and returns `Mat4`. |
| `lookAt` | `(eye: Vec3D, target: Vec3D, up: Vec3D): Mat4` | Invokes `lookAt` and returns `Mat4`. |
| `mul` | `(a: Mat4, b: Mat4): Mat4` | Invokes `mul` and returns `Mat4`. |
| `transpose` | `(a: Mat4): Mat4` | Invokes `transpose` and returns `Mat4`. |
| `transformPoint` | `(mat: Mat4, p: Vec3D): Vec3D` | Invokes `transformPoint` and returns `Vec3D`. |
| `transformDir` | `(mat: Mat4, v: Vec3D): Vec3D` | Invokes `transformDir` and returns `Vec3D`. |
| `approxEq` | `(a: Mat4, b: Mat4, eps: f64): bool` | Invokes `approxEq` and returns `bool`. |

## Minimal Example

```cp
import { absF64 } from "gl-mat4";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
