# gl-mat3 — AI Agent Quick Ref

Zero-allocation 3x3 row-major matrix and 2D affine transform library with built-in pure-CP sin/cos for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Mat3,
    Vec2D,
    m3Abs,
    m3WrapPi,
    m3Sin,
    m3Cos,
    zero,
    identity,
} from "gl-mat3";
```

## Core Types & Constants

```cp
class Mat3 { m: [9]f64 }
class Vec2D { x: f64, y: f64 }
```

## API Signatures

```cp
fn m3Abs(x: f64): f64;
fn m3WrapPi(rad: f64): f64;
fn m3Sin(rad: f64): f64;
fn m3Cos(rad: f64): f64;
fn zero(): Mat3;
fn identity(): Mat3;
fn fromTranslation(tx: f64, ty: f64): Mat3;
fn fromScale(sx: f64, sy: f64): Mat3;
fn fromRotation(radians: f64): Mat3;
fn mul(a: Mat3, b: Mat3): Mat3;
fn transpose(a: Mat3): Mat3;
fn determinant(a: Mat3): f64;
fn isInvertible(a: Mat3): bool;
fn inverse(a: Mat3): Mat3;
fn transformPoint(mat: Mat3, p: Vec2D): Vec2D;
fn transformDir(mat: Mat3, v: Vec2D): Vec2D;
fn approxEq(a: Mat3, b: Mat3, eps: f64): bool;
```

## Minimal Usage

```cp
import { isInvertible } from "gl-mat3";

pub fn main() {
    let result = isInvertible(/* a: Mat3 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"gl-mat3"`; install via `cap add gl-mat3`.
