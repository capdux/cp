# lerp — AI Agent Quick Ref

Zero-heap interpolation, range remapping, angle conversion, and easing curves in pure CP (`v1.0.0`, `MIT`)

## Import

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

## API Signatures

```cp
fn absF64(x: f64): f64;
fn minF64(a: f64, b: f64): f64;
fn maxF64(a: f64, b: f64): f64;
fn clamp(x: f64, lo: f64, hi: f64): f64;
fn clamp01(x: f64): f64;
fn approxEq(a: f64, b: f64, eps: f64): bool;
fn lerp(a: f64, b: f64, t: f64): f64;
fn lerpClamped(a: f64, b: f64, t: f64): f64;
fn inverseLerp(a: f64, b: f64, value: f64): f64;
fn remap(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64;
fn remapClamped(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64;
fn smoothstep(edge0: f64, edge1: f64, x: f64): f64;
fn smootherstep(edge0: f64, edge1: f64, x: f64): f64;
fn step(edge: f64, x: f64): f64;
fn wrap(x: f64, lo: f64, hi: f64): f64;
fn pingPong(t: f64, length: f64): f64;
fn degToRad(deg: f64): f64;
fn radToDeg(rad: f64): f64;
fn easeInQuad(t: f64): f64;
fn easeOutQuad(t: f64): f64;
fn easeInOutQuad(t: f64): f64;
fn easeInCubic(t: f64): f64;
fn easeOutCubic(t: f64): f64;
fn easeInOutCubic(t: f64): f64;
```

## Minimal Usage

```cp
import { absF64 } from "lerp";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lerp"`; install via `cap add lerp`.
