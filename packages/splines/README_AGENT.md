# splines — AI Agent Quick Ref

Zero-heap Natural Cubic Spline (Thomas O(N)), Catmull-Rom spline, quadratic/cubic Bezier curves, smoothstep, and linear interpolation. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Vec2,
    CubicSpline,
    lerp,
    smoothstep,
    bezierQuad,
    bezierCubic,
    catmullRom,
} from "splines";
```

## Core Types & Constants

```cp
class Vec2 { x: f64, y: f64 }
class CubicSpline { xs: [64]f64, a: [64]f64, b: [64]f64, c: [64]f64, /* +3 fields */ } // methods: build(...) +5
```

## API Signatures

```cp
fn lerp(a: f64, b: f64, t: f64): f64;
fn smoothstep(edge0: f64, edge1: f64, x: f64): f64;
fn bezierQuad(p0: Vec2, p1: Vec2, p2: Vec2, t: f64): Vec2;
fn bezierCubic(p0: Vec2, p1: Vec2, p2: Vec2, p3: Vec2, t: f64): Vec2;
fn catmullRom(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;
```

## Minimal Usage

```cp
import { lerp } from "splines";

pub fn main() {
    let result = lerp(0.0, 0.0, 0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"splines"`; install via `cap add splines`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
