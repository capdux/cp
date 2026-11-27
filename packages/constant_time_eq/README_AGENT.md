# constant_time_eq — AI Agent Quick Ref

Zero-allocation constant-time cryptographic comparison, branchless selection/swap, and secure memory zeroization primitives (Rust subtle + zeroize parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Choice,
    maskU8,
    maskU32,
    maskU64,
    ctIsZeroU8,
    ctEqU8,
    ctEqU32,
    ctEqU64,
} from "constant_time_eq";
```

## Core Types & Constants

```cp
class Choice { val: u8 } // methods: fromU8(...) +6
```

## API Signatures

```cp
fn maskU8(c: Choice): u8;
fn maskU32(c: Choice): u32;
fn maskU64(c: Choice): u64;
fn ctIsZeroU8(x: u8): Choice;
fn ctEqU8(a: u8, b: u8): Choice;
fn ctEqU32(a: u32, b: u32): Choice;
fn ctEqU64(a: u64, b: u64): Choice;
fn ctLtU64(a: u64, b: u64): Choice;
fn ctGtU64(a: u64, b: u64): Choice;
fn ctSelectU8(c: Choice, when_true: u8, when_false: u8): u8;
fn ctSelectU32(c: Choice, when_true: u32, when_false: u32): u32;
fn ctSelectU64(c: Choice, when_true: u64, when_false: u64): u64;
fn ctSwapU64(c: Choice, a: &mut u64, b: &mut u64);
fn ctEqBytes(a: string, b: string): Choice;
fn ctEqFixed32(a: [32]u8, b: [32]u8): Choice;
fn ctCopyIf(c: Choice, dst: []u8, src: string);
fn zeroizeBytes(buf: []u8);
fn isAllZero(buf: string): Choice;
```

## Minimal Usage

```cp
import { isAllZero } from "constant_time_eq";

pub fn main() {
    let result = isAllZero("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"constant_time_eq"`; install via `cap add constant_time_eq`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
