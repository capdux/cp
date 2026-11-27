# fraction.js — AI Agent Quick Ref

Zero-heap exact rational number (fraction) arithmetic in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Rational,
    absToU64,
    divTrunc64,
    gcd64,
    zero,
    one,
    fromInt,
    new,
} from "fraction.js";
```

## Core Types & Constants

```cp
class Rational { numer: i64, denom: i64 }
```

## API Signatures

```cp
fn absToU64(v: i64): u64;
fn divTrunc64(a: i64, b: i64): i64;
fn gcd64(a: i64, b: i64): i64;
fn zero(): Rational;
fn one(): Rational;
fn fromInt(n: i64): Rational;
fn new(n: i64, d: i64): Rational;
fn neg(a: Rational): Rational;
fn abs(a: Rational): Rational;
fn recip(a: Rational): Rational;
fn add(a: Rational, b: Rational): Rational;
fn sub(a: Rational, b: Rational): Rational;
fn mul(a: Rational, b: Rational): Rational;
fn div(a: Rational, b: Rational): Rational;
fn eq(a: Rational, b: Rational): bool;
fn cmp(a: Rational, b: Rational): i32;
fn toF64(a: Rational): f64;
fn floorInt(a: Rational): i64;
fn ceilInt(a: Rational): i64;
fn isInteger(a: Rational): bool;
```

## Minimal Usage

```cp
import { isInteger } from "fraction.js";

pub fn main() {
    let result = isInteger(/* a: Rational */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fraction.js"`; install via `cap add fraction.js`.
