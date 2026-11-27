[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# point-in-polygon

Zero-heap 2D polygon geometry (Shoelace area, centroid, perimeter, ray-casting containment, convexity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:point-in-polygon` ([https://github.com/substack/point-in-polygon](https://github.com/substack/point-in-polygon)) / `crates.io:geo-types` ([https://github.com/georust/geo](https://github.com/georust/geo))（原库名 `point-in-polygon` → CP 包名 `point-in-polygon`）

| Metadata | Value |
| --- | --- |
| Package | `point-in-polygon` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add point-in-polygon
```

```cp
import {
    Pt2,
    absF64,
    sqrtF64,
    signedArea,
    area,
    isCounterClockwise,
    perimeter,
    centroid,
} from "point-in-polygon";
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
| `signedArea` | `(pts: &[]Pt2): f64` | Invokes `signedArea` and returns `f64`. |
| `area` | `(pts: &[]Pt2): f64` | Invokes `area` and returns `f64`. |
| `isCounterClockwise` | `(pts: &[]Pt2): bool` | Checks predicate `isCounterClockwise` and returns `bool`. |
| `perimeter` | `(pts: &[]Pt2): f64` | Invokes `perimeter` and returns `f64`. |
| `centroid` | `(pts: &[]Pt2): Pt2` | Invokes `centroid` and returns `Pt2`. |
| `pointOnSegment` | `(p: Pt2, a: Pt2, b: Pt2): bool` | Invokes `pointOnSegment` and returns `bool`. |
| `containsPoint` | `(pts: &[]Pt2, p: Pt2): bool` | Invokes `containsPoint` and returns `bool`. |
| `isConvex` | `(pts: &[]Pt2): bool` | Checks predicate `isConvex` and returns `bool`. |

## Minimal Example

```cp
import { isCounterClockwise } from "point-in-polygon";

pub fn main() {
    let result = isCounterClockwise(/* pts: &[]Pt2 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
