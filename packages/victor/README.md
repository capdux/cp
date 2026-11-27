[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# victor

Zero-allocation 2D vector math and planar geometry library with built-in pure-CP sqrt/sin/cos for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:victor` ([https://github.com/maxkueng/victor](https://github.com/maxkueng/victor)) / `crates.io:euclid` ([https://github.com/servo/euclid](https://github.com/servo/euclid))（原库名 `victor` → CP 包名 `victor`）

| Metadata | Value |
| --- | --- |
| Package | `victor` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add victor
```

```cp
import {
    Vec2,
    v2Abs,
    v2Sqrt,
    v2WrapPi,
    v2Sin,
    v2Cos,
    new,
    zero,
} from "victor";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Vec2` | `class` | Fields: `x: f64, y: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `v2Abs` | `(x: f64): f64` | Invokes `v2Abs` and returns `f64`. |
| `v2Sqrt` | `(x: f64): f64` | Invokes `v2Sqrt` and returns `f64`. |
| `v2WrapPi` | `(rad: f64): f64` | Invokes `v2WrapPi` and returns `f64`. |
| `v2Sin` | `(rad: f64): f64` | Invokes `v2Sin` and returns `f64`. |
| `v2Cos` | `(rad: f64): f64` | Invokes `v2Cos` and returns `f64`. |
| `new` | `(x: f64, y: f64): Vec2` | Constructs/initializes an instance via `new`. |
| `zero` | `(): Vec2` | Invokes `zero` and returns `Vec2`. |
| `splat` | `(v: f64): Vec2` | Invokes `splat` and returns `Vec2`. |
| `add` | `(a: Vec2, b: Vec2): Vec2` | Mutates or appends state via `add`. |
| `sub` | `(a: Vec2, b: Vec2): Vec2` | Invokes `sub` and returns `Vec2`. |
| `mul` | `(a: Vec2, b: Vec2): Vec2` | Invokes `mul` and returns `Vec2`. |
| `scale` | `(v: Vec2, s: f64): Vec2` | Invokes `scale` and returns `Vec2`. |
| `neg` | `(v: Vec2): Vec2` | Invokes `neg` and returns `Vec2`. |
| `dot` | `(a: Vec2, b: Vec2): f64` | Invokes `dot` and returns `f64`. |
| `cross` | `(a: Vec2, b: Vec2): f64` | Invokes `cross` and returns `f64`. |
| `lengthSq` | `(v: Vec2): f64` | Invokes `lengthSq` and returns `f64`. |
| `length` | `(v: Vec2): f64` | Invokes `length` and returns `f64`. |
| `distance` | `(a: Vec2, b: Vec2): f64` | Invokes `distance` and returns `f64`. |
| `normalize` | `(v: Vec2): Vec2` | Invokes `normalize` and returns `Vec2`. |
| `lerp` | `(a: Vec2, b: Vec2, t: f64): Vec2` | Invokes `lerp` and returns `Vec2`. |
| `perp` | `(v: Vec2): Vec2` | Invokes `perp` and returns `Vec2`. |
| `reflect` | `(v: Vec2, normal: Vec2): Vec2` | Invokes `reflect` and returns `Vec2`. |
| `project` | `(a: Vec2, onto: Vec2): Vec2` | Invokes `project` and returns `Vec2`. |
| `rotate` | `(v: Vec2, radians: f64): Vec2` | Invokes `rotate` and returns `Vec2`. |
| `clampLength` | `(v: Vec2, max_len: f64): Vec2` | Invokes `clampLength` and returns `Vec2`. |
| `approxEq` | `(a: Vec2, b: Vec2, eps: f64): bool` | Invokes `approxEq` and returns `bool`. |

## Minimal Example

```cp
import { v2Abs } from "victor";

pub fn main() {
    let result = v2Abs(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
