# gl-matrix — AI Agent Quick Ref

Zero-allocation 3D vector and spatial geometry library for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Vec3,
    sqrtF64,
    absF64,
    minF64,
    maxF64,
    new,
    zero,
    splat,
} from "gl-matrix";
```

## Core Types & Constants

```cp
class Vec3 { x: f64, y: f64, z: f64 } // methods: new(...) +23
```

## API Signatures

```cp
fn sqrtF64(x: f64): f64;
fn absF64(x: f64): f64;
fn minF64(a: f64, b: f64): f64;
fn maxF64(a: f64, b: f64): f64;
fn new(x: f64, y: f64, z: f64): Vec3;
fn zero(): Vec3;
fn splat(v: f64): Vec3;
fn unitX(): Vec3;
fn unitY(): Vec3;
fn unitZ(): Vec3;
fn add(a: Vec3, b: Vec3): Vec3;
fn sub(a: Vec3, b: Vec3): Vec3;
fn mul(a: Vec3, b: Vec3): Vec3;
fn scale(v: Vec3, s: f64): Vec3;
fn neg(v: Vec3): Vec3;
fn dot(a: Vec3, b: Vec3): f64;
fn cross(a: Vec3, b: Vec3): Vec3;
fn lengthSq(v: Vec3): f64;
fn length(v: Vec3): f64;
fn distance(a: Vec3, b: Vec3): f64;
fn normalize(v: Vec3): Vec3;
fn lerp(a: Vec3, b: Vec3, t: f64): Vec3;
fn reflect(v: Vec3, normal: Vec3): Vec3;
fn project(v: Vec3, onto: Vec3): Vec3;
fn minVec(a: Vec3, b: Vec3): Vec3;
fn maxVec(a: Vec3, b: Vec3): Vec3;
fn clampLength(v: Vec3, max_len: f64): Vec3;
fn approxEq(a: Vec3, b: Vec3, eps: f64): bool;
```

## Minimal Usage

```cp
import { sqrtF64 } from "gl-matrix";

pub fn main() {
    let result = sqrtF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"gl-matrix"`; install via `cap add gl-matrix`.
