[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# splines

Zero-heap Natural Cubic Spline (Thomas O(N)), Catmull-Rom spline, quadratic/cubic Bezier curves, smoothstep, and linear interpolation.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:splines` ([https://github.com/phaazon/splines](https://github.com/phaazon/splines))（原库名 `splines` → CP 包名 `splines`）

| Metadata | Value |
| --- | --- |
| Package | `splines` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add splines
```

```cp
import {
    Vec2,
    CubicSpline,
    lerp,
    smoothstep,
    bezierQuad,
    bezierCubic,
    catmullRom,
} from "splines";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Vec2` | `class` | Fields: `x: f64, y: f64` |
| `CubicSpline` | `class` | Fields: `xs: [64]f64, a: [64]f64, b: [64]f64, c: [64]f64, d: [64]f64, count: usize, +1 more` · Methods: `build, isValid, knotCount, findSegment, eval, derivative` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lerp` | `(a: f64, b: f64, t: f64): f64` | Invokes `lerp` and returns `f64`. |
| `smoothstep` | `(edge0: f64, edge1: f64, x: f64): f64` | Invokes `smoothstep` and returns `f64`. |
| `bezierQuad` | `(p0: Vec2, p1: Vec2, p2: Vec2, t: f64): Vec2` | Invokes `bezierQuad` and returns `Vec2`. |
| `bezierCubic` | `(p0: Vec2, p1: Vec2, p2: Vec2, p3: Vec2, t: f64): Vec2` | Invokes `bezierCubic` and returns `Vec2`. |
| `catmullRom` | `(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64` | Invokes `catmullRom` and returns `f64`. |

## Minimal Example

```cp
import { lerp } from "splines";

pub fn main() {
    let result = lerp(0.0, 0.0, 0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
