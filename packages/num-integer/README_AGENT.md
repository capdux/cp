# num-integer — AI Agent Quick Ref

Zero-allocation integer number theory, overflow-safe GCD/LCM/extended-GCD, floor/ceil division, isqrt/icbrt, and binomial coefficients for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ExtGcdResult,
    nintAbsU64,
    nintDivTruncRem,
    gcdU64,
    lcmU64,
    extendedGcd,
    divFloor,
    modFloor,
} from "num-integer";
```

## Core Types & Constants

```cp
class ExtGcdResult { gcd: i64, x: i64, y: i64 }
```

## API Signatures

```cp
fn nintAbsU64(v: i64): u64;
fn nintDivTruncRem(a: i64, b: i64, q_out: &mut i64, r_out: &mut i64);
fn gcdU64(a: u64, b: u64): u64;
fn lcmU64(a: u64, b: u64): u64;
fn extendedGcd(a: i64, b: i64): ExtGcdResult;
fn divFloor(a: i64, b: i64): i64;
fn modFloor(a: i64, b: i64): i64;
fn divCeil(a: i64, b: i64): i64;
fn isqrtU64(n: u64): u64;
fn icbrtU64(n: u64): u64;
fn binomialU64(n: u64, k: u64): u64;
fn isEven(n: i64): bool;
fn isOdd(n: i64): bool;
fn nextMultipleOfU64(n: u64, step: u64): u64;
fn prevMultipleOfU64(n: u64, step: u64): u64;
```

## Minimal Usage

```cp
import { isqrtU64 } from "num-integer";

pub fn main() {
    let result = isqrtU64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"num-integer"`; install via `cap add num-integer`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
