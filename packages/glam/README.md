[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# glam

Zero-heap 3D math library with Vec3, unit Quat, and column-major Mat4 (Rust glam parity, pure CP with built-in sqrt/sin/cos).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:glam` ([https://github.com/bitshifter/glam-rs](https://github.com/bitshifter/glam-rs))

| Metadata | Value |
| --- | --- |
| Package | `glam` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add glam
```

```cp
import {
    Vec3,
    Quat,
    Mat4,
    absF64,
    sqrtF64,
    sinF64,
    cosF64,
} from "glam";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Vec3` | `class` | Fields: `x: f64, y: f64, z: f64` · Methods: `init, add, sub, scale, dot, cross, +5 more` |
| `Quat` | `class` | Fields: `x: f64, y: f64, z: f64, w: f64` · Methods: `identity, fromAxisAngle, normalize, conjugate, mul, rotateVec3` |
| `Mat4` | `class` | Fields: `m: [16]f64` · Methods: `identity, translation, scaling, rotationZ, mul, transformPoint, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `sinF64` | `(angle: f64): f64` | Invokes `sinF64` and returns `f64`. |
| `cosF64` | `(angle: f64): f64` | Invokes `cosF64` and returns `f64`. |

## Minimal Example

```cp
import { absF64 } from "glam";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
