# subtle — AI Agent Quick Ref

Zero-allocation branchless constant-time cryptographic primitives (Choice, ctEq, ctLt, ctSelect, ctCondSwap, zeroizeBytes) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Choice,
    mask8,
    mask32,
    mask64,
    ctIsZeroU8,
    ctIsZeroU32,
    ctIsZeroU64,
    ctEqU8,
} from "subtle";
```

## Core Types & Constants

```cp
class Choice { val: u8 } // methods: fromU8(...) +5
```

## API Signatures

```cp
fn mask8(c: Choice): u8;
fn mask32(c: Choice): u32;
fn mask64(c: Choice): u64;
fn ctIsZeroU8(x: u8): Choice;
fn ctIsZeroU32(x: u32): Choice;
fn ctIsZeroU64(x: u64): Choice;
fn ctEqU8(a: u8, b: u8): Choice;
fn ctEqU32(a: u32, b: u32): Choice;
fn ctEqU64(a: u64, b: u64): Choice;
fn ctEqBytes(a: &[]u8, b: &[]u8): Choice;
fn ctLtU64(a: u64, b: u64): Choice;
fn ctGtU64(a: u64, b: u64): Choice;
fn ctSelectU8(a: u8, b: u8, c: Choice): u8;
fn ctSelectU32(a: u32, b: u32, c: Choice): u32;
fn ctSelectU64(a: u64, b: u64, c: Choice): u64;
fn ctSelectBytes(dst: []u8, a: &[]u8, b: &[]u8, c: Choice): usize;
fn ctCondSwapU64(a: &mut u64, b: &mut u64, c: Choice);
fn zeroizeBytes(buf: []u8);
```

## Minimal Usage

```cp
import { mask8 } from "subtle";

pub fn main() {
    let result = mask8(/* c: Choice */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"subtle"`; install via `cap add subtle`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
