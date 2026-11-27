# lerp@1.0.0 (Agent Quickref)

> Zero-heap interpolation, range remapping, angle conversion, and easing curves in pure CP | License: MIT
> Import: `import { ... } from "lerp";` | Full types: `src/index.d.cp`

## Functions
- `export fn absF64(x: f64): f64;`
- `export fn minF64(a: f64, b: f64): f64;`
- `export fn maxF64(a: f64, b: f64): f64;`
- `export fn clamp(x: f64, lo: f64, hi: f64): f64;`
- `export fn clamp01(x: f64): f64;`
- `export fn approxEq(a: f64, b: f64, eps: f64): bool;`
- `export fn lerp(a: f64, b: f64, t: f64): f64;`
- `export fn lerpClamped(a: f64, b: f64, t: f64): f64;`
- `export fn inverseLerp(a: f64, b: f64, value: f64): f64;`
- `export fn remap(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64;`
- `export fn remapClamped(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64;`
- `export fn smoothstep(edge0: f64, edge1: f64, x: f64): f64;`
- `export fn smootherstep(edge0: f64, edge1: f64, x: f64): f64;`
- `export fn step(edge: f64, x: f64): f64;`
- `export fn wrap(x: f64, lo: f64, hi: f64): f64;`
- `export fn pingPong(t: f64, length: f64): f64;`
- `export fn degToRad(deg: f64): f64;`
- `export fn radToDeg(rad: f64): f64;`
- `export fn easeInQuad(t: f64): f64;`
- `export fn easeOutQuad(t: f64): f64;`
- `export fn easeInOutQuad(t: f64): f64;`
- `export fn easeInCubic(t: f64): f64;`
- `export fn easeOutCubic(t: f64): f64;`
- `export fn easeInOutCubic(t: f64): f64;`
