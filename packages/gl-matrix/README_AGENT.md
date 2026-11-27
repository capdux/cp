# gl-matrix@1.0.0 (Agent Quickref)

> Zero-allocation 3D vector and spatial geometry library for CP | License: MIT
> Import: `import { ... } from "gl-matrix";` | Full types: `src/index.d.cp`

## Functions
- `export fn sqrtF64(x: f64): f64;`
- `export fn absF64(x: f64): f64;`
- `export fn minF64(a: f64, b: f64): f64;`
- `export fn maxF64(a: f64, b: f64): f64;`
- `export fn new(x: f64, y: f64, z: f64): Vec3;`
- `export fn zero(): Vec3;`
- `export fn splat(v: f64): Vec3;`
- `export fn unitX(): Vec3;`
- `export fn unitY(): Vec3;`
- `export fn unitZ(): Vec3;`
- `export fn add(a: Vec3, b: Vec3): Vec3;`
- `export fn sub(a: Vec3, b: Vec3): Vec3;`
- `export fn mul(a: Vec3, b: Vec3): Vec3;`
- `export fn scale(v: Vec3, s: f64): Vec3;`
- `export fn neg(v: Vec3): Vec3;`
- `export fn dot(a: Vec3, b: Vec3): f64;`
- `export fn cross(a: Vec3, b: Vec3): Vec3;`
- `export fn lengthSq(v: Vec3): f64;`
- `export fn length(v: Vec3): f64;`
- `export fn distance(a: Vec3, b: Vec3): f64;`
- `export fn normalize(v: Vec3): Vec3;`
- `export fn lerp(a: Vec3, b: Vec3, t: f64): Vec3;`
- `export fn reflect(v: Vec3, normal: Vec3): Vec3;`
- `export fn project(v: Vec3, onto: Vec3): Vec3;`
- *(+4 more in `src/index.d.cp`)*
