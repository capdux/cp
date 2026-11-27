# d3-interpolate — AI Agent Quick Ref

Zero-allocation numeric, integer, RGB/HSL color, gamma-corrected, and uniform B-spline interpolators (`v3.0.1`, `ISC`)

## Import

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

## Core Types & Constants

```cp
class RgbColor { r: u8, g: u8, b: u8, a: f64 }
class HslColor { h: f64, s: f64, l: f64, a: f64 }
```

## API Signatures

```cp
fn d3iClampU8(v: f64): u8;
fn d3iClamp01(v: f64): f64;
fn interpolateNumber(a: f64, b: f64, t: f64): f64;
fn interpolateRound(a: i64, b: i64, t: f64): i64;
fn interpolateDiscrete(values: &[]f64, t: f64): f64;
fn interpolateRgb(c0: RgbColor, c1: RgbColor, t: f64): RgbColor;
fn interpolateHue(a: f64, b: f64, t: f64): f64;
fn interpolateHsl(c0: HslColor, c1: HslColor, t: f64): HslColor;
fn basis(t1: f64, v0: f64, v1: f64, v2: f64, v3: f64): f64;
fn interpolateBasis(values: &[]f64, t: f64): f64;
fn interpolatePiecewise(values: &[]f64, t: f64): f64;
```

## Minimal Usage

```cp
import { d3iClampU8 } from "d3-interpolate";

pub fn main() {
    let result = d3iClampU8(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"d3-interpolate"`; install via `cap add d3-interpolate`.
