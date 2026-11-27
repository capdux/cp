[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mint

Zero-heap Math Interoperability Standard Types: Point2/3, Vector2/3/4, Quaternion, RowMatrix2/3/4, ColumnMatrix4, EulerAngles (crates.io mint equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:mint` ([https://github.com/kvark/mint](https://github.com/kvark/mint))

| Metadata | Value |
| --- | --- |
| Package | `mint` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mint
```

```cp
import {
    Point2,
    Point3,
    Vector2,
    Vector3,
    Vector4,
    Quaternion,
    EulerAngles,
    RowMatrix2,
} from "mint";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Point2` | `class` | Fields: `x: f64, y: f64` · Methods: `new, fromSlice, toArray` |
| `Point3` | `class` | Fields: `x: f64, y: f64, z: f64` · Methods: `new, toArray` |
| `Vector2` | `class` | Fields: `x: f64, y: f64` · Methods: `new, dot, lenSq, toArray` |
| `Vector3` | `class` | Fields: `x: f64, y: f64, z: f64` · Methods: `new, dot, cross, lenSq, toArray` |
| `Vector4` | `class` | Fields: `x: f64, y: f64, z: f64, w: f64` · Methods: `new, dot, toArray` |
| `Quaternion` | `class` | Fields: `v: Vector3, s: f64` · Methods: `identity, new, conjugate, normSq, mul` |
| `EulerAngles` | `class` | Fields: `a: f64, b: f64, c: f64` · Methods: `new` |
| `RowMatrix2` | `class` | Fields: `x: Vector2, y: Vector2` · Methods: `identity, det, transpose` |
| `RowMatrix3` | `class` | Fields: `x: Vector3, y: Vector3, z: Vector3` · Methods: `identity, transpose, mulVec3` |
| `RowMatrix4` | `class` | Fields: `x: Vector4, y: Vector4, z: Vector4, w: Vector4` · Methods: `identity, transpose, mulVec4, toColumnMajor` |
| `ColumnMatrix4` | `class` | Fields: `x: Vector4, y: Vector4, z: Vector4, w: Vector4` · Methods: `identity, toRowMajor` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `point3ToVector3` | `(p: Point3): Vector3` | Invokes `point3ToVector3` and returns `Vector3`. |
| `vector3ToPoint3` | `(v: Vector3): Point3` | Invokes `vector3ToPoint3` and returns `Point3`. |

## Minimal Example

```cp
import { point3ToVector3 } from "mint";

pub fn main() {
    let result = point3ToVector3(/* p: Point3 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
