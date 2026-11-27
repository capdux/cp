[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ezing

Zero-allocation Robert Penner animation easing functions and CSS cubic-bezier solver (Rust ezing / keyframe / npm eases parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ezing` ([https://github.com/NyxCode/ezing](https://github.com/NyxCode/ezing))（原库名 `ezing` → CP 包名 `ezing`）

| Metadata | Value |
| --- | --- |
| Package | `ezing` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ezing
```

```cp
import {
    clamp01,
    absF64,
    sqrtF64,
    sinF64,
    cosF64,
    linear,
    quadIn,
    quadOut,
} from "ezing";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `clamp01` | `(t: f64): f64` | Invokes `clamp01` and returns `f64`. |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `sinF64` | `(x_in: f64): f64` | Invokes `sinF64` and returns `f64`. |
| `cosF64` | `(x: f64): f64` | Invokes `cosF64` and returns `f64`. |
| `linear` | `(t: f64): f64` | Invokes `linear` and returns `f64`. |
| `quadIn` | `(t_in: f64): f64` | Invokes `quadIn` and returns `f64`. |
| `quadOut` | `(t_in: f64): f64` | Invokes `quadOut` and returns `f64`. |
| `quadInOut` | `(t_in: f64): f64` | Invokes `quadInOut` and returns `f64`. |
| `cubicIn` | `(t_in: f64): f64` | Invokes `cubicIn` and returns `f64`. |
| `cubicOut` | `(t_in: f64): f64` | Invokes `cubicOut` and returns `f64`. |
| `cubicInOut` | `(t_in: f64): f64` | Invokes `cubicInOut` and returns `f64`. |
| `quartIn` | `(t_in: f64): f64` | Invokes `quartIn` and returns `f64`. |
| `quartOut` | `(t_in: f64): f64` | Invokes `quartOut` and returns `f64`. |
| `quartInOut` | `(t_in: f64): f64` | Invokes `quartInOut` and returns `f64`. |
| `quintIn` | `(t_in: f64): f64` | Invokes `quintIn` and returns `f64`. |
| `quintOut` | `(t_in: f64): f64` | Invokes `quintOut` and returns `f64`. |
| `quintInOut` | `(t_in: f64): f64` | Invokes `quintInOut` and returns `f64`. |
| `sineIn` | `(t_in: f64): f64` | Invokes `sineIn` and returns `f64`. |
| `sineOut` | `(t_in: f64): f64` | Invokes `sineOut` and returns `f64`. |
| `sineInOut` | `(t_in: f64): f64` | Invokes `sineInOut` and returns `f64`. |
| `circIn` | `(t_in: f64): f64` | Invokes `circIn` and returns `f64`. |
| `circOut` | `(t_in: f64): f64` | Invokes `circOut` and returns `f64`. |
| `circInOut` | `(t_in: f64): f64` | Invokes `circInOut` and returns `f64`. |
| `backIn` | `(t_in: f64): f64` | Invokes `backIn` and returns `f64`. |
| `backOut` | `(t_in: f64): f64` | Invokes `backOut` and returns `f64`. |
| `backInOut` | `(t_in: f64): f64` | Invokes `backInOut` and returns `f64`. |
| `bounceOut` | `(t_in: f64): f64` | Invokes `bounceOut` and returns `f64`. |
| `bounceIn` | `(t_in: f64): f64` | Invokes `bounceIn` and returns `f64`. |
| `bounceInOut` | `(t_in: f64): f64` | Invokes `bounceInOut` and returns `f64`. |
| `sampleCurve` | `(a: f64, b: f64, c: f64, u: f64): f64` | Invokes `sampleCurve` and returns `f64`. |
| `sampleCurveDeriv` | `(a: f64, b: f64, c: f64, u: f64): f64` | Invokes `sampleCurveDeriv` and returns `f64`. |
| `cubicBezier` | `(x1: f64, y1: f64, x2: f64, y2: f64, x_in: f64): f64` | Invokes `cubicBezier` and returns `f64`. |

## Minimal Example

```cp
import { clamp01 } from "ezing";

pub fn main() {
    let result = clamp01(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
