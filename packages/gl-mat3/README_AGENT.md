# gl-mat3@1.0.0 (Agent Quickref)

> Zero-allocation 3x3 row-major matrix and 2D affine transform library with built-in pure-CP sin/cos for CP. | License: MIT
> Import: `import { ... } from "gl-mat3";` | Full types: `src/index.d.cp`

## Functions
- `export fn zero(): Mat3;`
- `export fn identity(): Mat3;`
- `export fn fromTranslation(tx: f64, ty: f64): Mat3;`
- `export fn fromScale(sx: f64, sy: f64): Mat3;`
- `export fn fromRotation(radians: f64): Mat3;`
- `export fn mul(a: Mat3, b: Mat3): Mat3;`
- `export fn transpose(a: Mat3): Mat3;`
- `export fn determinant(a: Mat3): f64;`
- `export fn isInvertible(a: Mat3): bool;`
- `export fn inverse(a: Mat3): Mat3;`
- `export fn transformPoint(mat: Mat3, p: Vec2D): Vec2D;`
- `export fn transformDir(mat: Mat3, v: Vec2D): Vec2D;`
- `export fn approxEq(a: Mat3, b: Mat3, eps: f64): bool;`
