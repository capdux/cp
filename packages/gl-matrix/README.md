[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# gl-matrix

Zero-allocation 3D vector and spatial geometry library for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:gl-matrix` ([https://github.com/toji/gl-matrix](https://github.com/toji/gl-matrix))（原库名 `gl-matrix` → CP 包名 `gl-matrix`）

| Metadata | Value |
| --- | --- |
| Package | `gl-matrix` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add gl-matrix
```

```cp
import {
    Vec3,
    sqrtF64,
    absF64,
    minF64,
    maxF64,
    new,
    zero,
    splat,
} from "gl-matrix";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Vec3` | `class` | Fields: `x: f64, y: f64, z: f64` · Methods: `new, zero, splat, unitX, unitY, unitZ, +18 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `minF64` | `(a: f64, b: f64): f64` | Invokes `minF64` and returns `f64`. |
| `maxF64` | `(a: f64, b: f64): f64` | Invokes `maxF64` and returns `f64`. |
| `new` | `(x: f64, y: f64, z: f64): Vec3` | Constructs/initializes an instance via `new`. |
| `zero` | `(): Vec3` | Invokes `zero` and returns `Vec3`. |
| `splat` | `(v: f64): Vec3` | Invokes `splat` and returns `Vec3`. |
| `unitX` | `(): Vec3` | Invokes `unitX` and returns `Vec3`. |
| `unitY` | `(): Vec3` | Invokes `unitY` and returns `Vec3`. |
| `unitZ` | `(): Vec3` | Invokes `unitZ` and returns `Vec3`. |
| `add` | `(a: Vec3, b: Vec3): Vec3` | Mutates or appends state via `add`. |
| `sub` | `(a: Vec3, b: Vec3): Vec3` | Invokes `sub` and returns `Vec3`. |
| `mul` | `(a: Vec3, b: Vec3): Vec3` | Invokes `mul` and returns `Vec3`. |
| `scale` | `(v: Vec3, s: f64): Vec3` | Invokes `scale` and returns `Vec3`. |
| `neg` | `(v: Vec3): Vec3` | Invokes `neg` and returns `Vec3`. |
| `dot` | `(a: Vec3, b: Vec3): f64` | Invokes `dot` and returns `f64`. |
| `cross` | `(a: Vec3, b: Vec3): Vec3` | Invokes `cross` and returns `Vec3`. |
| `lengthSq` | `(v: Vec3): f64` | Invokes `lengthSq` and returns `f64`. |
| `length` | `(v: Vec3): f64` | Invokes `length` and returns `f64`. |
| `distance` | `(a: Vec3, b: Vec3): f64` | Invokes `distance` and returns `f64`. |
| `normalize` | `(v: Vec3): Vec3` | Invokes `normalize` and returns `Vec3`. |
| `lerp` | `(a: Vec3, b: Vec3, t: f64): Vec3` | Invokes `lerp` and returns `Vec3`. |
| `reflect` | `(v: Vec3, normal: Vec3): Vec3` | Invokes `reflect` and returns `Vec3`. |
| `project` | `(v: Vec3, onto: Vec3): Vec3` | Invokes `project` and returns `Vec3`. |
| `minVec` | `(a: Vec3, b: Vec3): Vec3` | Invokes `minVec` and returns `Vec3`. |
| `maxVec` | `(a: Vec3, b: Vec3): Vec3` | Invokes `maxVec` and returns `Vec3`. |
| `clampLength` | `(v: Vec3, max_len: f64): Vec3` | Invokes `clampLength` and returns `Vec3`. |
| `approxEq` | `(a: Vec3, b: Vec3, eps: f64): bool` | Invokes `approxEq` and returns `bool`. |

## Minimal Example

```cp
import { sqrtF64 } from "gl-matrix";

pub fn main() {
    let result = sqrtF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
