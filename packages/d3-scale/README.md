[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# d3-scale

Zero-allocation D3 quantitative scales (LinearScale, PowScale, SqrtScale, LogScale, QuantizeScale) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-scale` ([https://github.com/d3/d3-scale](https://github.com/d3/d3-scale))

| Metadata | Value |
| --- | --- |
| Package | `d3-scale` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add d3-scale
```

```cp
import {
    LinearScale,
    PowScale,
    SqrtScale,
    LogScale,
    QuantizeScale,
    d3scClamp01,
    d3scLerp,
    d3scSignedPow,
} from "d3-scale";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LinearScale` | `class` | Fields: `d0: f64, d1: f64, r0: f64, r1: f64, clamped: bool` · Methods: `clamp, apply, invert, ticksCount, ticks` |
| `PowScale` | `class` | Fields: `d0: f64, d1: f64, r0: f64, r1: f64, exponent: f64, clamped: bool` · Methods: `clamp, apply, invert` |
| `SqrtScale` | `class` | Fields: `d0: f64, d1: f64, r0: f64, r1: f64, clamped: bool` · Methods: `clamp, apply, invert` |
| `LogScale` | `class` | Fields: `d0: f64, d1: f64, r0: f64, r1: f64, base: f64, clamped: bool` · Methods: `clamp, apply, invert` |
| `QuantizeScale` | `class` | Fields: `d0: f64, d1: f64, buckets: usize` · Methods: `applyIndex, applyValue, invertExtentMin, invertExtentMax` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `d3scClamp01` | `(t: f64): f64` | Invokes `d3scClamp01` and returns `f64`. |
| `d3scLerp` | `(a: f64, b: f64, t: f64): f64` | Invokes `d3scLerp` and returns `f64`. |
| `d3scSignedPow` | `(x: f64, exp: f64): f64` | Invokes `d3scSignedPow` and returns `f64`. |
| `d3scTickIncrement` | `(start: f64, stop: f64, count: usize): f64` | Invokes `d3scTickIncrement` and returns `f64`. |
| `linearScale` | `(d0: f64, d1: f64, r0: f64, r1: f64): LinearScale` | Invokes `linearScale` and returns `LinearScale`. |
| `powScale` | `(d0: f64, d1: f64, r0: f64, r1: f64, exponent: f64): PowScale` | Invokes `powScale` and returns `PowScale`. |
| `sqrtScale` | `(d0: f64, d1: f64, r0: f64, r1: f64): SqrtScale` | Invokes `sqrtScale` and returns `SqrtScale`. |
| `logScale` | `(d0: f64, d1: f64, r0: f64, r1: f64, base: f64): LogScale` | Invokes `logScale` and returns `LogScale`. |
| `quantizeScale` | `(d0: f64, d1: f64, buckets: usize): QuantizeScale` | Invokes `quantizeScale` and returns `QuantizeScale`. |

## Minimal Example

```cp
import { d3scClamp01 } from "d3-scale";

pub fn main() {
    let result = d3scClamp01(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
