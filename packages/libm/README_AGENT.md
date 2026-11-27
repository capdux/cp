# libm — AI Agent Quick Ref

Pure CP zero-allocation IEEE 754 f64 math library (libm parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    LmTrigRed,
    lmNan,
    lmInf,
    lmScalePow2,
    fabs,
    trunc,
    floor,
    ceil,
} from "libm";
```

## Core Types & Constants

```cp
class LmTrigRed { r: f64, quad: i64 }
```

## API Signatures

```cp
fn lmNan(): f64;
fn lmInf(): f64;
fn lmScalePow2(val: f64, exp2: i64): f64;
fn fabs(x: f64): f64;
fn trunc(x: f64): f64;
fn floor(x: f64): f64;
fn ceil(x: f64): f64;
fn round(x: f64): f64;
fn fmod(x: f64, y: f64): f64;
fn copysign(x: f64, y: f64): f64;
fn fmin(x: f64, y: f64): f64;
fn fmax(x: f64, y: f64): f64;
fn sqrt(x: f64): f64;
fn cbrt(x: f64): f64;
fn hypot(x: f64, y: f64): f64;
fn exp(x: f64): f64;
fn ln(x: f64): f64;
fn log2(x: f64): f64;
fn log10(x: f64): f64;
fn pow(x: f64, y: f64): f64;
fn lmSinCore(r: f64): f64;
fn lmCosCore(r: f64): f64;
fn lmReduceTrig(x: f64): LmTrigRed;
fn sin(x: f64): f64;
fn cos(x: f64): f64;
fn tan(x: f64): f64;
fn lmAtanUnit(u: f64): f64;
fn atan(x: f64): f64;
fn atan2(y: f64, x: f64): f64;
fn asin(x: f64): f64;
fn acos(x: f64): f64;
fn sinh(x: f64): f64;
fn cosh(x: f64): f64;
fn tanh(x: f64): f64;
```

## Minimal Usage

```cp
import { lmNan } from "libm";

pub fn main() {
    let result = lmNan();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"libm"`; install via `cap add libm`.
