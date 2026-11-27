# rand_xoshiro — AI Agent Quick Ref

Zero-allocation Blackman & Vigna xoshiro256** PRNG with SplitMix64 seeding, unbiased nextRange, 2^128 jump, and Fisher-Yates shuffle in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Xoshiro256,
    makeU64,
    rotl7,
    rotl45,
    splitmix64Next,
    seedFromU64,
    nextU64,
    nextF64,
} from "rand_xoshiro";
```

## Core Types & Constants

```cp
class Xoshiro256 { s: [4]u64 }
```

## API Signatures

```cp
fn makeU64(hi: u32, lo: u32): u64;
fn rotl7(x: u64): u64;
fn rotl45(x: u64): u64;
fn splitmix64Next(state: &mut u64): u64;
fn seedFromU64(seed: u64): Xoshiro256;
fn nextU64(rng: &mut Xoshiro256): u64;
fn nextF64(rng: &mut Xoshiro256): f64;
fn nextRange(rng: &mut Xoshiro256, upper_exclusive: u64): u64;
fn jump(rng: &mut Xoshiro256);
fn shuffleU32(rng: &mut Xoshiro256, data: []u32);
```

## Minimal Usage

```cp
import { makeU64 } from "rand_xoshiro";

pub fn main() {
    let result = makeU64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rand_xoshiro"`; install via `cap add rand_xoshiro`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
