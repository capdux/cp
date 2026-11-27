# splines@1.0.0 (Agent Quickref)

> Zero-heap Natural Cubic Spline (Thomas O(N)), Catmull-Rom spline, quadratic/cubic Bezier curves, smoothstep, and linear interpolation. | License: MIT
> Import: `import { ... } from "splines";` | Full types: `src/index.d.cp`

## Functions
- `export fn lerp(a: f64, b: f64, t: f64): f64;`
- `export fn smoothstep(edge0: f64, edge1: f64, x: f64): f64;`
- `export fn bezierQuad(p0: Vec2, p1: Vec2, p2: Vec2, t: f64): Vec2;`
- `export fn bezierCubic(p0: Vec2, p1: Vec2, p2: Vec2, p3: Vec2, t: f64): Vec2;`
- `export fn catmullRom(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;`
