# @cap/num-traits

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-traits` ([https://github.com/rust-num/num-traits](https://github.com/rust-num/num-traits))

Clean-room MIT, zero-heap overflow-safe numeric arithmetic and bit-manipulation library for CP — modeled on crates.io #10 `num-traits` v0.2 (2B+ downloads).

## Features

- **Zero heap allocation**: pure stack/register arithmetic with no dynamic memory or panicking overflows.
- **Checked arithmetic**: returns `null` instead of trapping on signed/unsigned overflow or underflow.
- **Saturating arithmetic**: clamps results to `[0, U64_MAX]` or `[I64_MIN, I64_MAX]`.
- **Clamp, Sign, AbsDiff & Checked Pow**: safe range clamping (with NaN propagation for `f64`), signum, unsigned absolute difference, and binary exponentiation with overflow checks.
- **O(1) Bit Manipulation**: power-of-two checks, `nextPowerOfTwoU64`, SWAR `countOnesU64`, branch-free/binary-search `leadingZerosU64` & `trailingZerosU64`, and modulo-64 bit rotations.

## API

### Constants
- `I64_MAX: i64`
- `I64_MIN: i64`
- `U64_MAX: u64`

### Checked Arithmetic
- `checkedAddI64(a: i64, b: i64): i64?`
- `checkedSubI64(a: i64, b: i64): i64?`
- `checkedMulI64(a: i64, b: i64): i64?`
- `checkedAddU64(a: u64, b: u64): u64?`
- `checkedSubU64(a: u64, b: u64): u64?`
- `checkedMulU64(a: u64, b: u64): u64?`

### Saturating Arithmetic
- `saturatingAddU64(a: u64, b: u64): u64`
- `saturatingSubU64(a: u64, b: u64): u64`
- `saturatingMulU64(a: u64, b: u64): u64`
- `saturatingAddI64(a: i64, b: i64): i64`
- `saturatingSubI64(a: i64, b: i64): i64`

### Clamp / Sign / Pow
- `clampI64(v: i64, lo: i64, hi: i64): i64`
- `clampF64(v: f64, lo: f64, hi: f64): f64`
- `signumI64(v: i64): i64`
- `absDiffU64(a: u64, b: u64): u64`
- `checkedPowU64(base: u64, exp: u32): u64?`

### Bit Manipulation & Powers of Two
- `isPowerOfTwoU64(n: u64): bool`
- `nextPowerOfTwoU64(n: u64): u64?`
- `countOnesU64(n: u64): u32`
- `leadingZerosU64(n: u64): u32`
- `trailingZerosU64(n: u64): u32`
- `rotateLeftU64(n: u64, k: u32): u64`
- `rotateRightU64(n: u64, k: u32): u64`

## Usage

```typescript
import {
    checkedAddI64,
    saturatingSubU64,
    checkedPowU64,
    nextPowerOfTwoU64,
    rotateLeftU64,
} from "@cap/num-traits";

let sum = checkedAddI64(9223372036854775807, 1); // null (overflow)
let clamped = saturatingSubU64(5, 10);           // 0
let pow = checkedPowU64(2, 10);                  // 1024
let p2 = nextPowerOfTwoU64(17);                  // 32
let rot = rotateLeftU64(0x8000000000000001, 1);  // 0x0000000000000003
```

## License

MIT
