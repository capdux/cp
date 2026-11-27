# complex.js — AI Agent Quick Ref

Zero-allocation complex number arithmetic, polar form, exponential, integer powers, and principal square root for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Complex,
    absF64,
    sqrtF64,
    reduceAngle,
    sinF64,
    cosF64,
    expF64,
    atanSmall,
} from "complex.js";
```

## Core Types & Constants

```cp
class Complex { re: f64, im: f64 } // methods: new(...) +17
```

## API Signatures

```cp
fn absF64(x: f64): f64;
fn sqrtF64(x: f64): f64;
fn reduceAngle(rad: f64): f64;
fn sinF64(rad: f64): f64;
fn cosF64(rad: f64): f64;
fn expF64(x: f64): f64;
fn atanSmall(x: f64): f64;
fn atanF64(x: f64): f64;
fn atan2F64(y: f64, x: f64): f64;
fn new(re: f64, im: f64): Complex;
fn zero(): Complex;
fn one(): Complex;
fn iUnit(): Complex;
fn fromPolar(r: f64, theta: f64): Complex;
fn add(a: Complex, b: Complex): Complex;
fn sub(a: Complex, b: Complex): Complex;
fn mul(a: Complex, b: Complex): Complex;
fn div(a: Complex, b: Complex): Complex;
fn conj(z: Complex): Complex;
fn neg(z: Complex): Complex;
fn scale(z: Complex, s: f64): Complex;
fn normSq(z: Complex): f64;
fn norm(z: Complex): f64;
fn arg(z: Complex): f64;
fn exp(z: Complex): Complex;
fn powInt(z: Complex, n: i32): Complex;
fn sqrt(z: Complex): Complex;
fn approxEq(a: Complex, b: Complex, eps: f64): bool;
```

## Minimal Usage

```cp
import { absF64 } from "complex.js";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"complex.js"`; install via `cap add complex.js`.
