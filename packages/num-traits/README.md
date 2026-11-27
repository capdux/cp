[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# num-traits

Zero-heap overflow-safe numeric operations and bit manipulation primitives: checked/saturating arithmetic, clamp, signum, absDiff, checkedPow, powers of two, popcount, clz, ctz, and rotations (clean-room port of Rust num-traits)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-traits` ([https://github.com/rust-num/num-traits](https://github.com/rust-num/num-traits))

| Metadata | Value |
| --- | --- |
| Package | `num-traits` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add num-traits
```

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

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ntrtAbsI64ToU64` | `(x: i64): u64` | Invokes `ntrtAbsI64ToU64` and returns `u64`. |
| `checkedAddI64` | `(a: i64, b: i64): i64?` | Validates input with `checkedAddI64` returning `i64?`. |
| `checkedSubI64` | `(a: i64, b: i64): i64?` | Validates input with `checkedSubI64` returning `i64?`. |
| `checkedMulI64` | `(a: i64, b: i64): i64?` | Validates input with `checkedMulI64` returning `i64?`. |
| `checkedAddU64` | `(a: u64, b: u64): u64?` | Validates input with `checkedAddU64` returning `u64?`. |
| `checkedSubU64` | `(a: u64, b: u64): u64?` | Validates input with `checkedSubU64` returning `u64?`. |
| `checkedMulU64` | `(a: u64, b: u64): u64?` | Validates input with `checkedMulU64` returning `u64?`. |
| `saturatingAddU64` | `(a: u64, b: u64): u64` | Invokes `saturatingAddU64` and returns `u64`. |
| `saturatingSubU64` | `(a: u64, b: u64): u64` | Invokes `saturatingSubU64` and returns `u64`. |
| `saturatingMulU64` | `(a: u64, b: u64): u64` | Invokes `saturatingMulU64` and returns `u64`. |
| `saturatingAddI64` | `(a: i64, b: i64): i64` | Invokes `saturatingAddI64` and returns `i64`. |
| `saturatingSubI64` | `(a: i64, b: i64): i64` | Invokes `saturatingSubI64` and returns `i64`. |
| `clampI64` | `(v: i64, lo: i64, hi: i64): i64` | Invokes `clampI64` and returns `i64`. |
| `clampF64` | `(v: f64, lo: f64, hi: f64): f64` | Invokes `clampF64` and returns `f64`. |
| `signumI64` | `(v: i64): i64` | Invokes `signumI64` and returns `i64`. |
| `absDiffU64` | `(a: u64, b: u64): u64` | Invokes `absDiffU64` and returns `u64`. |
| `checkedPowU64` | `(base: u64, exp: u32): u64?` | Validates input with `checkedPowU64` returning `u64?`. |
| `isPowerOfTwoU64` | `(n: u64): bool` | Checks predicate `isPowerOfTwoU64` and returns `bool`. |
| `nextPowerOfTwoU64` | `(n: u64): u64?` | Invokes `nextPowerOfTwoU64` and returns `u64?`. |
| `countOnesU64` | `(n: u64): u32` | Invokes `countOnesU64` and returns `u32`. |
| `leadingZerosU64` | `(n: u64): u32` | Invokes `leadingZerosU64` and returns `u32`. |
| `trailingZerosU64` | `(n: u64): u32` | Invokes `trailingZerosU64` and returns `u32`. |
| `rotateLeftU64` | `(n: u64, k: u32): u64` | Invokes `rotateLeftU64` and returns `u64`. |
| `rotateRightU64` | `(n: u64, k: u32): u64` | Invokes `rotateRightU64` and returns `u64`. |

## Minimal Example

```cp
import { isPowerOfTwoU64 } from "num-traits";

pub fn main() {
    let result = isPowerOfTwoU64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
