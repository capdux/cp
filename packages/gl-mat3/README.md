[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# gl-mat3

Zero-allocation 3x3 row-major matrix and 2D affine transform library with built-in pure-CP sin/cos for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:gl-mat3` ([https://github.com/stackgl/gl-mat3](https://github.com/stackgl/gl-mat3)) / `crates.io:cgmath` ([https://github.com/rustgd/cgmath](https://github.com/rustgd/cgmath))（原库名 `gl-mat3` → CP 包名 `gl-mat3`）

| Metadata | Value |
| --- | --- |
| Package | `gl-mat3` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add gl-mat3
```

```cp
import {
    Mat3,
    Vec2D,
    m3Abs,
    m3WrapPi,
    m3Sin,
    m3Cos,
    zero,
    identity,
} from "gl-mat3";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Mat3` | `class` | Fields: `m: [9]f64` |
| `Vec2D` | `class` | Fields: `x: f64, y: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `m3Abs` | `(x: f64): f64` | Invokes `m3Abs` and returns `f64`. |
| `m3WrapPi` | `(rad: f64): f64` | Invokes `m3WrapPi` and returns `f64`. |
| `m3Sin` | `(rad: f64): f64` | Invokes `m3Sin` and returns `f64`. |
| `m3Cos` | `(rad: f64): f64` | Invokes `m3Cos` and returns `f64`. |
| `zero` | `(): Mat3` | Invokes `zero` and returns `Mat3`. |
| `identity` | `(): Mat3` | Invokes `identity` and returns `Mat3`. |
| `fromTranslation` | `(tx: f64, ty: f64): Mat3` | Invokes `fromTranslation` and returns `Mat3`. |
| `fromScale` | `(sx: f64, sy: f64): Mat3` | Invokes `fromScale` and returns `Mat3`. |
| `fromRotation` | `(radians: f64): Mat3` | Invokes `fromRotation` and returns `Mat3`. |
| `mul` | `(a: Mat3, b: Mat3): Mat3` | Invokes `mul` and returns `Mat3`. |
| `transpose` | `(a: Mat3): Mat3` | Invokes `transpose` and returns `Mat3`. |
| `determinant` | `(a: Mat3): f64` | Invokes `determinant` and returns `f64`. |
| `isInvertible` | `(a: Mat3): bool` | Checks predicate `isInvertible` and returns `bool`. |
| `inverse` | `(a: Mat3): Mat3` | Invokes `inverse` and returns `Mat3`. |
| `transformPoint` | `(mat: Mat3, p: Vec2D): Vec2D` | Invokes `transformPoint` and returns `Vec2D`. |
| `transformDir` | `(mat: Mat3, v: Vec2D): Vec2D` | Invokes `transformDir` and returns `Vec2D`. |
| `approxEq` | `(a: Mat3, b: Mat3, eps: f64): bool` | Invokes `approxEq` and returns `bool`. |

## Minimal Example

```cp
import { isInvertible } from "gl-mat3";

pub fn main() {
    let result = isInvertible(/* a: Mat3 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
