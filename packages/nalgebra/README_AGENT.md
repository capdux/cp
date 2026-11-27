# nalgebra — AI Agent Quick Ref

Zero-heap 2D/3D/4D linear algebra: Mat2/Mat3/Mat4 multiply, determinant, inverse, affine transforms & vector ops (crates.io nalgebra equivalent, reuses mint) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    naSqrt,
    vec2Add,
    vec2Sub,
    vec2Scale,
    vec2Norm,
    vec3Add,
    vec3Sub,
    vec3Scale,
} from "nalgebra";
```

## API Signatures

```cp
fn naSqrt(x: f64): f64;
fn vec2Add(a: Vector2, b: Vector2): Vector2;
fn vec2Sub(a: Vector2, b: Vector2): Vector2;
fn vec2Scale(v: Vector2, s: f64): Vector2;
fn vec2Norm(v: Vector2): f64;
fn vec3Add(a: Vector3, b: Vector3): Vector3;
fn vec3Sub(a: Vector3, b: Vector3): Vector3;
fn vec3Scale(v: Vector3, s: f64): Vector3;
fn vec3Norm(v: Vector3): f64;
fn vec3Normalize(v: Vector3): Vector3?;
fn vec3Lerp(a: Vector3, b: Vector3, t: f64): Vector3;
fn vec3Distance(a: Vector3, b: Vector3): f64;
fn mat2Mul(a: RowMatrix2, b: RowMatrix2): RowMatrix2;
fn mat2Inverse(m: RowMatrix2): RowMatrix2?;
fn mat3Det(m: RowMatrix3): f64;
fn mat3Mul(a: RowMatrix3, b: RowMatrix3): RowMatrix3;
fn mat3Inverse(m: RowMatrix3): RowMatrix3?;
fn mat4Mul(a: RowMatrix4, b: RowMatrix4): RowMatrix4;
fn mat4Translation(tx: f64, ty: f64, tz: f64): RowMatrix4;
fn mat4Scaling(sx: f64, sy: f64, sz: f64): RowMatrix4;
fn mat4TransformPoint(m: RowMatrix4, p: Vector3): Vector3;
```

## Minimal Usage

```cp
import { naSqrt } from "nalgebra";

pub fn main() {
    let result = naSqrt(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"nalgebra"`; install via `cap add nalgebra`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
