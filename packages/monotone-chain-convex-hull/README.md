[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# monotone-chain-convex-hull

Zero-heap 2D Andrew's monotone chain convex hull, area, and perimeter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:monotone-chain-convex-hull` ([https://github.com/mikolalysenko/monotone-chain-convex-hull](https://github.com/mikolalysenko/monotone-chain-convex-hull))（原库名 `monotone-chain-convex-hull` → CP 包名 `monotone-chain-convex-hull`）

| Metadata | Value |
| --- | --- |
| Package | `monotone-chain-convex-hull` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add monotone-chain-convex-hull
```

```cp
import {
    Pt2,
    absF64,
    sqrtF64,
    ptLess,
    ptEqual,
    crossOAB,
    sortPointsXY,
    convexHull,
} from "monotone-chain-convex-hull";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Pt2` | `class` | Fields: `x: f64, y: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `ptLess` | `(a: Pt2, b: Pt2): bool` | Invokes `ptLess` and returns `bool`. |
| `ptEqual` | `(a: Pt2, b: Pt2): bool` | Invokes `ptEqual` and returns `bool`. |
| `crossOAB` | `(o: Pt2, a: Pt2, b: Pt2): f64` | Invokes `crossOAB` and returns `f64`. |
| `sortPointsXY` | `(pts: []Pt2)` | Executes `sortPointsXY` in-place. |
| `convexHull` | `(work_pts: []Pt2, out_hull: []Pt2): usize` | Invokes `convexHull` and returns `usize`. |
| `hullArea` | `(hull: &[]Pt2): f64` | Invokes `hullArea` and returns `f64`. |
| `hullPerimeter` | `(hull: &[]Pt2): f64` | Invokes `hullPerimeter` and returns `f64`. |

## Minimal Example

```cp
import { absF64 } from "monotone-chain-convex-hull";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
