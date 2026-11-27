# ezing — AI Agent Quick Ref

Zero-allocation Robert Penner animation easing functions and CSS cubic-bezier solver (Rust ezing / keyframe / npm eases parity) for CP (`v1.0.0`, `MIT`)

## Import

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

## API Signatures

```cp
fn clamp01(t: f64): f64;
fn absF64(x: f64): f64;
fn sqrtF64(x: f64): f64;
fn sinF64(x_in: f64): f64;
fn cosF64(x: f64): f64;
fn linear(t: f64): f64;
fn quadIn(t_in: f64): f64;
fn quadOut(t_in: f64): f64;
fn quadInOut(t_in: f64): f64;
fn cubicIn(t_in: f64): f64;
fn cubicOut(t_in: f64): f64;
fn cubicInOut(t_in: f64): f64;
fn quartIn(t_in: f64): f64;
fn quartOut(t_in: f64): f64;
fn quartInOut(t_in: f64): f64;
fn quintIn(t_in: f64): f64;
fn quintOut(t_in: f64): f64;
fn quintInOut(t_in: f64): f64;
fn sineIn(t_in: f64): f64;
fn sineOut(t_in: f64): f64;
fn sineInOut(t_in: f64): f64;
fn circIn(t_in: f64): f64;
fn circOut(t_in: f64): f64;
fn circInOut(t_in: f64): f64;
fn backIn(t_in: f64): f64;
fn backOut(t_in: f64): f64;
fn backInOut(t_in: f64): f64;
fn bounceOut(t_in: f64): f64;
fn bounceIn(t_in: f64): f64;
fn bounceInOut(t_in: f64): f64;
fn sampleCurve(a: f64, b: f64, c: f64, u: f64): f64;
fn sampleCurveDeriv(a: f64, b: f64, c: f64, u: f64): f64;
fn cubicBezier(x1: f64, y1: f64, x2: f64, y2: f64, x_in: f64): f64;
```

## Minimal Usage

```cp
import { clamp01 } from "ezing";

pub fn main() {
    let result = clamp01(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ezing"`; install via `cap add ezing`.
