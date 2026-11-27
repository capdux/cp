# num-traits — AI Agent Quick Ref

Zero-heap overflow-safe numeric operations and bit manipulation primitives: checked/saturating arithmetic, clamp, signum, absDiff, checkedPow, powers of two, popcount, clz, ctz, and rotations (clean-room port of Rust num-traits) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ntrtAbsI64ToU64,
    checkedAddI64,
    checkedSubI64,
    checkedMulI64,
    checkedAddU64,
    checkedSubU64,
    checkedMulU64,
    saturatingAddU64,
} from "num-traits";
```

## API Signatures

```cp
fn ntrtAbsI64ToU64(x: i64): u64;
fn checkedAddI64(a: i64, b: i64): i64?;
fn checkedSubI64(a: i64, b: i64): i64?;
fn checkedMulI64(a: i64, b: i64): i64?;
fn checkedAddU64(a: u64, b: u64): u64?;
fn checkedSubU64(a: u64, b: u64): u64?;
fn checkedMulU64(a: u64, b: u64): u64?;
fn saturatingAddU64(a: u64, b: u64): u64;
fn saturatingSubU64(a: u64, b: u64): u64;
fn saturatingMulU64(a: u64, b: u64): u64;
fn saturatingAddI64(a: i64, b: i64): i64;
fn saturatingSubI64(a: i64, b: i64): i64;
fn clampI64(v: i64, lo: i64, hi: i64): i64;
fn clampF64(v: f64, lo: f64, hi: f64): f64;
fn signumI64(v: i64): i64;
fn absDiffU64(a: u64, b: u64): u64;
fn checkedPowU64(base: u64, exp: u32): u64?;
fn isPowerOfTwoU64(n: u64): bool;
fn nextPowerOfTwoU64(n: u64): u64?;
fn countOnesU64(n: u64): u32;
fn leadingZerosU64(n: u64): u32;
fn trailingZerosU64(n: u64): u32;
fn rotateLeftU64(n: u64, k: u32): u64;
fn rotateRightU64(n: u64, k: u32): u64;
```

## Minimal Usage

```cp
import { isPowerOfTwoU64 } from "num-traits";

pub fn main() {
    let result = isPowerOfTwoU64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"num-traits"`; install via `cap add num-traits`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
