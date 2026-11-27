[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# parry2d

Zero-allocation 2D axis-aligned bounding box (AABB) collision detection, union, intersection, and ray slab intersection for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:parry2d` ([https://github.com/dimforge/parry](https://github.com/dimforge/parry))（原库名 `parry2d` → CP 包名 `parry2d`）

| Metadata | Value |
| --- | --- |
| Package | `parry2d` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add parry2d
```

```cp
import {
    Pt2,
    Aabb2,
    minF64,
    maxF64,
    new,
    fromCenterSize,
    fromPoints,
} from "parry2d";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Pt2` | `class` | Fields: `x: f64, y: f64` · Methods: `new` |
| `Aabb2` | `class` | Fields: `min_x: f64, min_y: f64, max_x: f64, max_y: f64` · Methods: `new, empty, fromCenterSize, fromPoints, isValid, width, +13 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `minF64` | `(a: f64, b: f64): f64` | Invokes `minF64` and returns `f64`. |
| `maxF64` | `(a: f64, b: f64): f64` | Invokes `maxF64` and returns `f64`. |
| `new` | `(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Aabb2` | Constructs/initializes an instance via `new`. |
| `fromCenterSize` | `(cx: f64, cy: f64, w: f64, h: f64): Aabb2` | Invokes `fromCenterSize` and returns `Aabb2`. |
| `fromPoints` | `(pts: &[]Pt2): Aabb2` | Invokes `fromPoints` and returns `Aabb2`. |

## Minimal Example

```cp
import { minF64 } from "parry2d";

pub fn main() {
    let result = minF64(0.0, 0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
