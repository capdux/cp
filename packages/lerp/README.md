[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# lerp

Zero-heap interpolation, range remapping, angle conversion, and easing curves in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lerp` ([https://github.com/mattdesl/lerp](https://github.com/mattdesl/lerp)) / `npm:eases` ([https://github.com/mattdesl/eases](https://github.com/mattdesl/eases)) / `npm:smoothstep` ([https://github.com/mikolalysenko/smoothstep](https://github.com/mikolalysenko/smoothstep)) / `crates.io:interpolation` ([https://github.com/PistonDevelopers/interpolation](https://github.com/PistonDevelopers/interpolation)) / `crates.io:keyframe` ([https://github.com/HannesMann/keyframe](https://github.com/HannesMann/keyframe))（原库名 `lerp` → CP 包名 `lerp`）

| Metadata | Value |
| --- | --- |
| Package | `lerp` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add lerp
```

```cp
import {
    absF64,
    minF64,
    maxF64,
    clamp,
    clamp01,
    approxEq,
    lerp,
    lerpClamped,
} from "lerp";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `minF64` | `(a: f64, b: f64): f64` | Invokes `minF64` and returns `f64`. |
| `maxF64` | `(a: f64, b: f64): f64` | Invokes `maxF64` and returns `f64`. |
| `clamp` | `(x: f64, lo: f64, hi: f64): f64` | Invokes `clamp` and returns `f64`. |
| `clamp01` | `(x: f64): f64` | Invokes `clamp01` and returns `f64`. |
| `approxEq` | `(a: f64, b: f64, eps: f64): bool` | Invokes `approxEq` and returns `bool`. |
| `lerp` | `(a: f64, b: f64, t: f64): f64` | Invokes `lerp` and returns `f64`. |
| `lerpClamped` | `(a: f64, b: f64, t: f64): f64` | Invokes `lerpClamped` and returns `f64`. |
| `inverseLerp` | `(a: f64, b: f64, value: f64): f64` | Invokes `inverseLerp` and returns `f64`. |
| `remap` | `(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64` | Invokes `remap` and returns `f64`. |
| `remapClamped` | `(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64` | Invokes `remapClamped` and returns `f64`. |
| `smoothstep` | `(edge0: f64, edge1: f64, x: f64): f64` | Invokes `smoothstep` and returns `f64`. |
| `smootherstep` | `(edge0: f64, edge1: f64, x: f64): f64` | Invokes `smootherstep` and returns `f64`. |
| `step` | `(edge: f64, x: f64): f64` | Invokes `step` and returns `f64`. |
| `wrap` | `(x: f64, lo: f64, hi: f64): f64` | Invokes `wrap` and returns `f64`. |
| `pingPong` | `(t: f64, length: f64): f64` | Invokes `pingPong` and returns `f64`. |
| `degToRad` | `(deg: f64): f64` | Invokes `degToRad` and returns `f64`. |
| `radToDeg` | `(rad: f64): f64` | Invokes `radToDeg` and returns `f64`. |
| `easeInQuad` | `(t: f64): f64` | Invokes `easeInQuad` and returns `f64`. |
| `easeOutQuad` | `(t: f64): f64` | Invokes `easeOutQuad` and returns `f64`. |
| `easeInOutQuad` | `(t: f64): f64` | Invokes `easeInOutQuad` and returns `f64`. |
| `easeInCubic` | `(t: f64): f64` | Invokes `easeInCubic` and returns `f64`. |
| `easeOutCubic` | `(t: f64): f64` | Invokes `easeOutCubic` and returns `f64`. |
| `easeInOutCubic` | `(t: f64): f64` | Invokes `easeInOutCubic` and returns `f64`. |

## Minimal Example

```cp
import { absF64 } from "lerp";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
