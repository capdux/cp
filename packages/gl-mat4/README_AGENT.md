# gl-mat4 — AI Agent Quick Ref

Zero-allocation 4x4 3D graphics transformation matrix library (translation, scale, rotation, perspective, orthographic, lookAt) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Vec3D,
    Mat4,
    absF64,
    sqrtF64,
    reduceAngle,
    sinF64,
    cosF64,
    tanF64,
} from "gl-mat4";
```

## Core Types & Constants

```cp
class Vec3D { x: f64, y: f64, z: f64 } // methods: new(...) +1
class Mat4 { m: [16]f64 } // methods: zero(...) +9
```

## API Signatures

```cp
fn absF64(x: f64): f64;
fn sqrtF64(x: f64): f64;
fn reduceAngle(rad: f64): f64;
fn sinF64(rad: f64): f64;
fn cosF64(rad: f64): f64;
fn tanF64(rad: f64): f64;
fn v3Sub(a: Vec3D, b: Vec3D): Vec3D;
fn v3Dot(a: Vec3D, b: Vec3D): f64;
fn v3Cross(a: Vec3D, b: Vec3D): Vec3D;
fn v3Normalize(v: Vec3D): Vec3D;
fn identity(): Mat4;
fn zero(): Mat4;
fn fromTranslation(tx: f64, ty: f64, tz: f64): Mat4;
fn fromScale(sx: f64, sy: f64, sz: f64): Mat4;
fn fromRotationX(rad: f64): Mat4;
fn fromRotationY(rad: f64): Mat4;
fn fromRotationZ(rad: f64): Mat4;
fn orthographic(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64): Mat4;
fn perspective(fov_y_rad: f64, aspect: f64, near: f64, far: f64): Mat4;
fn lookAt(eye: Vec3D, target: Vec3D, up: Vec3D): Mat4;
fn mul(a: Mat4, b: Mat4): Mat4;
fn transpose(a: Mat4): Mat4;
fn transformPoint(mat: Mat4, p: Vec3D): Vec3D;
fn transformDir(mat: Mat4, v: Vec3D): Vec3D;
fn approxEq(a: Mat4, b: Mat4, eps: f64): bool;
```

## Minimal Usage

```cp
import { absF64 } from "gl-mat4";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"gl-mat4"`; install via `cap add gl-mat4`.
