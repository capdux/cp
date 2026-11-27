[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# d3-interpolate

Zero-allocation numeric, integer, RGB/HSL color, gamma-corrected, and uniform B-spline interpolators

| Metadata | Value |
| --- | --- |
| Package | `d3-interpolate` |
| Version | `3.0.1` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add d3-interpolate
```

```cp
import {
    RgbColor,
    HslColor,
    d3iClampU8,
    d3iClamp01,
    interpolateNumber,
    interpolateRound,
    interpolateDiscrete,
    interpolateRgb,
} from "d3-interpolate";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RgbColor` | `class` | Fields: `r: u8, g: u8, b: u8, a: f64` |
| `HslColor` | `class` | Fields: `h: f64, s: f64, l: f64, a: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `d3iClampU8` | `(v: f64): u8` | Invokes `d3iClampU8` and returns `u8`. |
| `d3iClamp01` | `(v: f64): f64` | Invokes `d3iClamp01` and returns `f64`. |
| `interpolateNumber` | `(a: f64, b: f64, t: f64): f64` | Invokes `interpolateNumber` and returns `f64`. |
| `interpolateRound` | `(a: i64, b: i64, t: f64): i64` | Invokes `interpolateRound` and returns `i64`. |
| `interpolateDiscrete` | `(values: &[]f64, t: f64): f64` | Invokes `interpolateDiscrete` and returns `f64`. |
| `interpolateRgb` | `(c0: RgbColor, c1: RgbColor, t: f64): RgbColor` | Invokes `interpolateRgb` and returns `RgbColor`. |
| `interpolateHue` | `(a: f64, b: f64, t: f64): f64` | Invokes `interpolateHue` and returns `f64`. |
| `interpolateHsl` | `(c0: HslColor, c1: HslColor, t: f64): HslColor` | Invokes `interpolateHsl` and returns `HslColor`. |
| `basis` | `(t1: f64, v0: f64, v1: f64, v2: f64, v3: f64): f64` | Invokes `basis` and returns `f64`. |
| `interpolateBasis` | `(values: &[]f64, t: f64): f64` | Invokes `interpolateBasis` and returns `f64`. |
| `interpolatePiecewise` | `(values: &[]f64, t: f64): f64` | Invokes `interpolatePiecewise` and returns `f64`. |

## Minimal Example

```cp
import { d3iClampU8 } from "d3-interpolate";

pub fn main() {
    let result = d3iClampU8(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
