# quaternion — AI Agent Quick Ref

Zero-heap 3D unit quaternion rotation, SLERP/NLERP, and Euler conversion in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Quat,
    Point3,
    absF64,
    sqrtF64,
    wrapAngle,
    sinF64,
    cosF64,
    atanF64,
} from "quaternion";
```

## Core Types & Constants

```cp
class Quat { x: f64, y: f64, z: f64, w: f64 }
class Point3 { x: f64, y: f64, z: f64 }
```

## API Signatures

```cp
fn absF64(x: f64): f64;
fn sqrtF64(x: f64): f64;
fn wrapAngle(x: f64): f64;
fn sinF64(x: f64): f64;
fn cosF64(x: f64): f64;
fn atanF64(x: f64): f64;
fn acosF64(x: f64): f64;
fn new(x: f64, y: f64, z: f64, w: f64): Quat;
fn identity(): Quat;
fn fromAxisAngle(ax: f64, ay: f64, az: f64, radians: f64): Quat;
fn fromEulerXYZ(rx: f64, ry: f64, rz: f64): Quat;
fn mul(a: Quat, b: Quat): Quat;
fn conjugate(q: Quat): Quat;
fn dot(a: Quat, b: Quat): f64;
fn length(q: Quat): f64;
fn inverse(q: Quat): Quat;
fn normalize(q: Quat): Quat;
fn nlerp(a: Quat, b: Quat, t: f64): Quat;
fn slerp(a: Quat, b: Quat, t: f64): Quat;
fn rotatePoint(q: Quat, p: Point3): Point3;
fn approxEq(a: Quat, b: Quat, eps: f64): bool;
```

## Minimal Usage

```cp
import { absF64 } from "quaternion";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"quaternion"`; install via `cap add quaternion`.
