[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# nalgebra

Zero-heap 2D/3D/4D linear algebra: Mat2/Mat3/Mat4 multiply, determinant, inverse, affine transforms & vector ops (crates.io nalgebra equivalent, reuses mint)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:nalgebra` ([https://github.com/dimforge/nalgebra](https://github.com/dimforge/nalgebra))

| Metadata | Value |
| --- | --- |
| Package | `nalgebra` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add nalgebra
```

```cp
import {
    naSqrt,
    vec2Add,
    vec2Sub,
    vec2Scale,
    vec2Norm,
    vec3Add,
    vec3Sub,
    vec3Scale,
} from "nalgebra";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `naSqrt` | `(x: f64): f64` | Invokes `naSqrt` and returns `f64`. |
| `vec2Add` | `(a: Vector2, b: Vector2): Vector2` | Invokes `vec2Add` and returns `Vector2`. |
| `vec2Sub` | `(a: Vector2, b: Vector2): Vector2` | Invokes `vec2Sub` and returns `Vector2`. |
| `vec2Scale` | `(v: Vector2, s: f64): Vector2` | Invokes `vec2Scale` and returns `Vector2`. |
| `vec2Norm` | `(v: Vector2): f64` | Invokes `vec2Norm` and returns `f64`. |
| `vec3Add` | `(a: Vector3, b: Vector3): Vector3` | Invokes `vec3Add` and returns `Vector3`. |
| `vec3Sub` | `(a: Vector3, b: Vector3): Vector3` | Invokes `vec3Sub` and returns `Vector3`. |
| `vec3Scale` | `(v: Vector3, s: f64): Vector3` | Invokes `vec3Scale` and returns `Vector3`. |
| `vec3Norm` | `(v: Vector3): f64` | Invokes `vec3Norm` and returns `f64`. |
| `vec3Normalize` | `(v: Vector3): Vector3?` | Invokes `vec3Normalize` and returns `Vector3?`. |
| `vec3Lerp` | `(a: Vector3, b: Vector3, t: f64): Vector3` | Invokes `vec3Lerp` and returns `Vector3`. |
| `vec3Distance` | `(a: Vector3, b: Vector3): f64` | Invokes `vec3Distance` and returns `f64`. |
| `mat2Mul` | `(a: RowMatrix2, b: RowMatrix2): RowMatrix2` | Invokes `mat2Mul` and returns `RowMatrix2`. |
| `mat2Inverse` | `(m: RowMatrix2): RowMatrix2?` | Invokes `mat2Inverse` and returns `RowMatrix2?`. |
| `mat3Det` | `(m: RowMatrix3): f64` | Invokes `mat3Det` and returns `f64`. |
| `mat3Mul` | `(a: RowMatrix3, b: RowMatrix3): RowMatrix3` | Invokes `mat3Mul` and returns `RowMatrix3`. |
| `mat3Inverse` | `(m: RowMatrix3): RowMatrix3?` | Invokes `mat3Inverse` and returns `RowMatrix3?`. |
| `mat4Mul` | `(a: RowMatrix4, b: RowMatrix4): RowMatrix4` | Invokes `mat4Mul` and returns `RowMatrix4`. |
| `mat4Translation` | `(tx: f64, ty: f64, tz: f64): RowMatrix4` | Invokes `mat4Translation` and returns `RowMatrix4`. |
| `mat4Scaling` | `(sx: f64, sy: f64, sz: f64): RowMatrix4` | Invokes `mat4Scaling` and returns `RowMatrix4`. |
| `mat4TransformPoint` | `(m: RowMatrix4, p: Vector3): Vector3` | Invokes `mat4TransformPoint` and returns `Vector3`. |

## Minimal Example

```cp
import { naSqrt } from "nalgebra";

pub fn main() {
    let result = naSqrt(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
