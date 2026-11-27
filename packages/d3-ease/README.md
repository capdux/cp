[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# d3-ease

Zero-allocation D3 easing functions (linear, quad, cubic, poly, sin, exp, circle, bounce, back, elastic) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-ease` ([https://github.com/d3/d3-ease](https://github.com/d3/d3-ease))

| Metadata | Value |
| --- | --- |
| Package | `d3-ease` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add d3-ease
```

```cp
import {
    deasTpmt,
    deasClampAmp,
    easeLinear,
    easeQuadIn,
    easeQuadOut,
    easeQuadInOut,
    easeCubicIn,
    easeCubicOut,
} from "d3-ease";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `deasTpmt` | `(x: f64): f64` | Invokes `deasTpmt` and returns `f64`. |
| `deasClampAmp` | `(a: f64): f64` | Invokes `deasClampAmp` and returns `f64`. |
| `easeLinear` | `(t: f64): f64` | Invokes `easeLinear` and returns `f64`. |
| `easeQuadIn` | `(t: f64): f64` | Invokes `easeQuadIn` and returns `f64`. |
| `easeQuadOut` | `(t: f64): f64` | Invokes `easeQuadOut` and returns `f64`. |
| `easeQuadInOut` | `(t: f64): f64` | Invokes `easeQuadInOut` and returns `f64`. |
| `easeCubicIn` | `(t: f64): f64` | Invokes `easeCubicIn` and returns `f64`. |
| `easeCubicOut` | `(t: f64): f64` | Invokes `easeCubicOut` and returns `f64`. |
| `easeCubicInOut` | `(t: f64): f64` | Invokes `easeCubicInOut` and returns `f64`. |
| `easePolyInExp` | `(t: f64, e: f64): f64` | Invokes `easePolyInExp` and returns `f64`. |
| `easePolyOutExp` | `(t: f64, e: f64): f64` | Invokes `easePolyOutExp` and returns `f64`. |
| `easePolyInOutExp` | `(t: f64, e: f64): f64` | Invokes `easePolyInOutExp` and returns `f64`. |
| `easePolyIn` | `(t: f64): f64` | Invokes `easePolyIn` and returns `f64`. |
| `easePolyOut` | `(t: f64): f64` | Invokes `easePolyOut` and returns `f64`. |
| `easePolyInOut` | `(t: f64): f64` | Invokes `easePolyInOut` and returns `f64`. |
| `easeSinIn` | `(t: f64): f64` | Invokes `easeSinIn` and returns `f64`. |
| `easeSinOut` | `(t: f64): f64` | Invokes `easeSinOut` and returns `f64`. |
| `easeSinInOut` | `(t: f64): f64` | Invokes `easeSinInOut` and returns `f64`. |
| `easeExpIn` | `(t: f64): f64` | Invokes `easeExpIn` and returns `f64`. |
| `easeExpOut` | `(t: f64): f64` | Invokes `easeExpOut` and returns `f64`. |
| `easeExpInOut` | `(t: f64): f64` | Invokes `easeExpInOut` and returns `f64`. |
| `easeCircleIn` | `(t: f64): f64` | Invokes `easeCircleIn` and returns `f64`. |
| `easeCircleOut` | `(t: f64): f64` | Invokes `easeCircleOut` and returns `f64`. |
| `easeCircleInOut` | `(t: f64): f64` | Invokes `easeCircleInOut` and returns `f64`. |
| `easeBounceOut` | `(t: f64): f64` | Invokes `easeBounceOut` and returns `f64`. |
| `easeBounceIn` | `(t: f64): f64` | Invokes `easeBounceIn` and returns `f64`. |
| `easeBounceInOut` | `(t: f64): f64` | Invokes `easeBounceInOut` and returns `f64`. |
| `easeBackInOvershoot` | `(t: f64, s: f64): f64` | Invokes `easeBackInOvershoot` and returns `f64`. |
| `easeBackOutOvershoot` | `(t: f64, s: f64): f64` | Invokes `easeBackOutOvershoot` and returns `f64`. |
| `easeBackInOutOvershoot` | `(t: f64, s: f64): f64` | Invokes `easeBackInOutOvershoot` and returns `f64`. |
| `easeBackIn` | `(t: f64): f64` | Invokes `easeBackIn` and returns `f64`. |
| `easeBackOut` | `(t: f64): f64` | Invokes `easeBackOut` and returns `f64`. |
| `easeBackInOut` | `(t: f64): f64` | Invokes `easeBackInOut` and returns `f64`. |
| `easeElasticInParams` | `(t: f64, a: f64, p: f64): f64` | Invokes `easeElasticInParams` and returns `f64`. |
| `easeElasticOutParams` | `(t: f64, a: f64, p: f64): f64` | Invokes `easeElasticOutParams` and returns `f64`. |
| `easeElasticInOutParams` | `(t: f64, a: f64, p: f64): f64` | Invokes `easeElasticInOutParams` and returns `f64`. |
| `easeElasticIn` | `(t: f64): f64` | Invokes `easeElasticIn` and returns `f64`. |
| `easeElasticOut` | `(t: f64): f64` | Invokes `easeElasticOut` and returns `f64`. |
| `easeElasticInOut` | `(t: f64): f64` | Invokes `easeElasticInOut` and returns `f64`. |

## Minimal Example

```cp
import { deasTpmt } from "d3-ease";

pub fn main() {
    let result = deasTpmt(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
