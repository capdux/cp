[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# parry3d

Zero-allocation 3D Axis-Aligned Bounding Box (AABB) collision detection, union/intersection, surface area/volume, and Kay-Kajiya slab raycast library for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:parry3d` ([https://github.com/dimforge/parry](https://github.com/dimforge/parry)) / `npm:aabb-3d` ([https://github.com/chrisdickinson/aabb-3d](https://github.com/chrisdickinson/aabb-3d))（原库名 `parry3d` → CP 包名 `parry3d`）

| Metadata | Value |
| --- | --- |
| Package | `parry3d` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add parry3d
```

```cp
import {
    Pt3,
    Aabb3,
    abAbs,
    abMin,
    abMax,
} from "parry3d";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Pt3` | `class` | Fields: `x: f64, y: f64, z: f64` |
| `Aabb3` | `class` | Fields: `min_x: f64, min_y: f64, min_z: f64, max_x: f64, max_y: f64, max_z: f64` · Methods: `new, fromCenterSize, isValid, volume, surfaceArea, center, +7 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `abAbs` | `(x: f64): f64` | Invokes `abAbs` and returns `f64`. |
| `abMin` | `(a: f64, b: f64): f64` | Invokes `abMin` and returns `f64`. |
| `abMax` | `(a: f64, b: f64): f64` | Invokes `abMax` and returns `f64`. |

## Minimal Example

```cp
import { abAbs } from "parry3d";

pub fn main() {
    let result = abAbs(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
