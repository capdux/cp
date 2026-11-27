# victor — AI Agent Quick Ref

Zero-allocation 2D vector math and planar geometry library with built-in pure-CP sqrt/sin/cos for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Vec2,
    v2Abs,
    v2Sqrt,
    v2WrapPi,
    v2Sin,
    v2Cos,
    new,
    zero,
} from "victor";
```

## Core Types & Constants

```cp
class Vec2 { x: f64, y: f64 }
```

## API Signatures

```cp
fn v2Abs(x: f64): f64;
fn v2Sqrt(x: f64): f64;
fn v2WrapPi(rad: f64): f64;
fn v2Sin(rad: f64): f64;
fn v2Cos(rad: f64): f64;
fn new(x: f64, y: f64): Vec2;
fn zero(): Vec2;
fn splat(v: f64): Vec2;
fn add(a: Vec2, b: Vec2): Vec2;
fn sub(a: Vec2, b: Vec2): Vec2;
fn mul(a: Vec2, b: Vec2): Vec2;
fn scale(v: Vec2, s: f64): Vec2;
fn neg(v: Vec2): Vec2;
fn dot(a: Vec2, b: Vec2): f64;
fn cross(a: Vec2, b: Vec2): f64;
fn lengthSq(v: Vec2): f64;
fn length(v: Vec2): f64;
fn distance(a: Vec2, b: Vec2): f64;
fn normalize(v: Vec2): Vec2;
fn lerp(a: Vec2, b: Vec2, t: f64): Vec2;
fn perp(v: Vec2): Vec2;
fn reflect(v: Vec2, normal: Vec2): Vec2;
fn project(a: Vec2, onto: Vec2): Vec2;
fn rotate(v: Vec2, radians: f64): Vec2;
fn clampLength(v: Vec2, max_len: f64): Vec2;
fn approxEq(a: Vec2, b: Vec2, eps: f64): bool;
```

## Minimal Usage

```cp
import { v2Abs } from "victor";

pub fn main() {
    let result = v2Abs(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"victor"`; install via `cap add victor`.
