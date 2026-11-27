[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bezier-easing

Zero-allocation quadratic/cubic Bezier curves, de Casteljau splitting, arc length, and CSS cubic-bezier easing for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bezier-easing` ([https://github.com/gre/bezier-easing](https://github.com/gre/bezier-easing)) / `crates.io:kurbo` ([https://github.com/linebender/kurbo](https://github.com/linebender/kurbo)) / `crates.io:lyon_geom` ([https://github.com/nical/lyon](https://github.com/nical/lyon))（原库名 `bezier-easing` → CP 包名 `bezier-easing`）

| Metadata | Value |
| --- | --- |
| Package | `bezier-easing` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bezier-easing
```

```cp
import {
    Pt2,
    QuadBezier,
    CubicBezier,
    CubicSplit,
    sqrtF64,
    absF64,
    lerpPt2,
    distPt2,
} from "bezier-easing";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Pt2` | `class` | Fields: `x: f64, y: f64` · Methods: `new` |
| `QuadBezier` | `class` | Fields: `p0: Pt2, p1: Pt2, p2: Pt2` · Methods: `new, eval, deriv, arcLength` |
| `CubicBezier` | `class` | Fields: `p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2` · Methods: `new, eval, deriv, arcLength` |
| `CubicSplit` | `class` | Fields: `left: CubicBezier, right: CubicBezier` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `lerpPt2` | `(a: Pt2, b: Pt2, t: f64): Pt2` | Invokes `lerpPt2` and returns `Pt2`. |
| `distPt2` | `(a: Pt2, b: Pt2): f64` | Invokes `distPt2` and returns `f64`. |
| `splitCubic` | `(c: CubicBezier, t: f64): CubicSplit` | Invokes `splitCubic` and returns `CubicSplit`. |
| `evalUnitCubic` | `(p1: f64, p2: f64, t: f64): f64` | Invokes `evalUnitCubic` and returns `f64`. |
| `derivUnitCubic` | `(p1: f64, p2: f64, t: f64): f64` | Invokes `derivUnitCubic` and returns `f64`. |
| `cubicBezierEasing` | `(x1: f64, y1: f64, x2: f64, y2: f64, x: f64): f64` | Invokes `cubicBezierEasing` and returns `f64`. |

## Minimal Example

```cp
import { sqrtF64 } from "bezier-easing";

pub fn main() {
    let result = sqrtF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
