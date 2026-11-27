[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# quaternion

Zero-heap 3D unit quaternion rotation, SLERP/NLERP, and Euler conversion in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:quaternion` ([https://github.com/rawify/Quaternion.js](https://github.com/rawify/Quaternion.js))（原库名 `quaternion` → CP 包名 `quaternion`）

| Metadata | Value |
| --- | --- |
| Package | `quaternion` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add quaternion
```

```cp
import {
    Quat,
    Point3,
    absF64,
    sqrtF64,
    wrapAngle,
    sinF64,
    cosF64,
    atanF64,
} from "quaternion";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Quat` | `class` | Fields: `x: f64, y: f64, z: f64, w: f64` |
| `Point3` | `class` | Fields: `x: f64, y: f64, z: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `wrapAngle` | `(x: f64): f64` | Invokes `wrapAngle` and returns `f64`. |
| `sinF64` | `(x: f64): f64` | Invokes `sinF64` and returns `f64`. |
| `cosF64` | `(x: f64): f64` | Invokes `cosF64` and returns `f64`. |
| `atanF64` | `(x: f64): f64` | Invokes `atanF64` and returns `f64`. |
| `acosF64` | `(x: f64): f64` | Invokes `acosF64` and returns `f64`. |
| `new` | `(x: f64, y: f64, z: f64, w: f64): Quat` | Constructs/initializes an instance via `new`. |
| `identity` | `(): Quat` | Invokes `identity` and returns `Quat`. |
| `fromAxisAngle` | `(ax: f64, ay: f64, az: f64, radians: f64): Quat` | Invokes `fromAxisAngle` and returns `Quat`. |
| `fromEulerXYZ` | `(rx: f64, ry: f64, rz: f64): Quat` | Invokes `fromEulerXYZ` and returns `Quat`. |
| `mul` | `(a: Quat, b: Quat): Quat` | Invokes `mul` and returns `Quat`. |
| `conjugate` | `(q: Quat): Quat` | Invokes `conjugate` and returns `Quat`. |
| `dot` | `(a: Quat, b: Quat): f64` | Invokes `dot` and returns `f64`. |
| `length` | `(q: Quat): f64` | Invokes `length` and returns `f64`. |
| `inverse` | `(q: Quat): Quat` | Invokes `inverse` and returns `Quat`. |
| `normalize` | `(q: Quat): Quat` | Invokes `normalize` and returns `Quat`. |
| `nlerp` | `(a: Quat, b: Quat, t: f64): Quat` | Invokes `nlerp` and returns `Quat`. |
| `slerp` | `(a: Quat, b: Quat, t: f64): Quat` | Invokes `slerp` and returns `Quat`. |
| `rotatePoint` | `(q: Quat, p: Point3): Point3` | Invokes `rotatePoint` and returns `Point3`. |
| `approxEq` | `(a: Quat, b: Quat, eps: f64): bool` | Invokes `approxEq` and returns `bool`. |

## Minimal Example

```cp
import { absF64 } from "quaternion";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
