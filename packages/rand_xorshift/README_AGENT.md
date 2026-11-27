# rand_xorshift — AI Agent Quick Ref

Pure CP zero-allocation Marsaglia Xorshift PRNG family: XorShift128 (rand_xorshift parity), XorShift64Star, and XorShift32 (`v1.0.0`, `MIT`)

## Import

```cp
import {
    XorShift128,
    XorShift64Star,
    XorShift32,
    xsU64FromHalves,
    xsAddWrap64,
    xsMulWrap64,
    xsSplitMixNext,
    xsReadU32Le,
} from "rand_xorshift";
```

## Core Types & Constants

```cp
class XorShift128 { x: u32, y: u32, z: u32, w: u32 } // methods: fromSeed(...) +5
class XorShift64Star { state: u64 } // methods: new(...) +2
class XorShift32 { state: u32 } // methods: new(...) +1
```

## API Signatures

```cp
fn xsU64FromHalves(hi: u32, lo: u32): u64;
fn xsAddWrap64(a: u64, b: u64): u64;
fn xsMulWrap64(a: u64, b: u64): u64;
fn xsSplitMixNext(st: &mut u64): u64;
fn xsReadU32Le(b: [16]u8, off: usize): u32;
```

## Minimal Usage

```cp
import { xsU64FromHalves } from "rand_xorshift";

pub fn main() {
    let result = xsU64FromHalves(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rand_xorshift"`; install via `cap add rand_xorshift`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
