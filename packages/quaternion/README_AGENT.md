# quaternion@1.0.0 (Agent Quickref)

> CP package quaternion | License: MIT
> Import: `import { ... } from "quaternion";` | Full types: `src/index.d.cp`

## Functions
- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn wrapAngle(x: f64): f64;`
- `export fn sinF64(x: f64): f64;`
- `export fn cosF64(x: f64): f64;`
- `export fn atanF64(x: f64): f64;`
- `export fn acosF64(x: f64): f64;`
- `export fn new(x: f64, y: f64, z: f64, w: f64): Quat;`
- `export fn identity(): Quat;`
- `export fn fromAxisAngle(ax: f64, ay: f64, az: f64, radians: f64): Quat;`
- `export fn fromEulerXYZ(rx: f64, ry: f64, rz: f64): Quat;`
- `export fn mul(a: Quat, b: Quat): Quat;`
- `export fn conjugate(q: Quat): Quat;`
- `export fn dot(a: Quat, b: Quat): f64;`
- `export fn length(q: Quat): f64;`
- `export fn inverse(q: Quat): Quat;`
- `export fn normalize(q: Quat): Quat;`
- `export fn nlerp(a: Quat, b: Quat, t: f64): Quat;`
- `export fn slerp(a: Quat, b: Quat, t: f64): Quat;`
- `export fn rotatePoint(q: Quat, p: Point3): Point3;`
- `export fn approxEq(a: Quat, b: Quat, eps: f64): bool;`
