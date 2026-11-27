# bigdecimal — AI Agent Quick Ref

Zero-heap exact base-10 financial arithmetic library with 18-digit precision, rounding, parsing, and formatting (clean-room port of Rust bigdecimal) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BigDecimal,
    bdecIsSpace,
    bdecIsDigit,
    bdecAbsU64,
    bdecSignedFromU64,
    bdecPow10U64,
    bdecDigitCount,
    bdecWriteByte,
} from "bigdecimal";
```

## Core Types & Constants

```cp
class BigDecimal { digits: i64, scale: i32 } // methods: zero(...) +12
```

## API Signatures

```cp
fn bdecIsSpace(c: u8): bool;
fn bdecIsDigit(c: u8): bool;
fn bdecAbsU64(v: i64): u64;
fn bdecSignedFromU64(mag: u64, neg: bool): i64;
fn bdecPow10U64(exp: u32): u64;
fn bdecDigitCount(u: u64): i32;
fn bdecWriteByte(out: []u8, pos: usize, b: u8): usize;
fn parseDecimal(s: string): BigDecimal?;
fn formatDecimal(val: BigDecimal, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseDecimal } from "bigdecimal";

pub fn main() {
    let result = parseDecimal("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bigdecimal"`; install via `cap add bigdecimal`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
