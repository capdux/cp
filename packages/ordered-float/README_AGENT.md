# ordered-float — AI Agent Quick Ref

Zero-heap IEEE-754 total ordering, canonical hashing, and NaN-safe min/max/clamp for f64 and f32 (clean-room port of Rust ordered-float) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    OrderedF64,
    NotNanF64,
    isNanF64,
    isInfiniteF64,
    isFiniteF64,
    cmpOrderedF64,
    eqOrderedF64,
    hashOrderedF64,
} from "ordered-float";
```

## Core Types & Constants

```cp
class OrderedF64 { val: f64 } // methods: new(...) +9
class NotNanF64 { val: f64 } // methods: new(...) +4
```

## API Signatures

```cp
fn isNanF64(x: f64): bool;
fn isInfiniteF64(x: f64): bool;
fn isFiniteF64(x: f64): bool;
fn cmpOrderedF64(a: f64, b: f64): i32;
fn eqOrderedF64(a: f64, b: f64): bool;
fn hashOrderedF64(x: f64): u64;
fn sortOrderedF64(arr: []f64);
```

## Minimal Usage

```cp
import { isNanF64 } from "ordered-float";

pub fn main() {
    let result = isNanF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ordered-float"`; install via `cap add ordered-float`.
