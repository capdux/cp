[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# num-integer

Zero-allocation integer number theory, overflow-safe GCD/LCM/extended-GCD, floor/ceil division, isqrt/icbrt, and binomial coefficients for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-integer` ([https://github.com/rust-num/num-integer](https://github.com/rust-num/num-integer))

| Metadata | Value |
| --- | --- |
| Package | `num-integer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add num-integer
```

```cp
import {
    ExtGcdResult,
    nintAbsU64,
    nintDivTruncRem,
    gcdU64,
    lcmU64,
    extendedGcd,
    divFloor,
    modFloor,
} from "num-integer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ExtGcdResult` | `class` | Fields: `gcd: i64, x: i64, y: i64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nintAbsU64` | `(v: i64): u64` | Invokes `nintAbsU64` and returns `u64`. |
| `nintDivTruncRem` | `(a: i64, b: i64, q_out: &mut i64, r_out: &mut i64)` | Executes `nintDivTruncRem` in-place. |
| `gcdU64` | `(a: u64, b: u64): u64` | Invokes `gcdU64` and returns `u64`. |
| `lcmU64` | `(a: u64, b: u64): u64` | Invokes `lcmU64` and returns `u64`. |
| `extendedGcd` | `(a: i64, b: i64): ExtGcdResult` | Invokes `extendedGcd` and returns `ExtGcdResult`. |
| `divFloor` | `(a: i64, b: i64): i64` | Invokes `divFloor` and returns `i64`. |
| `modFloor` | `(a: i64, b: i64): i64` | Invokes `modFloor` and returns `i64`. |
| `divCeil` | `(a: i64, b: i64): i64` | Invokes `divCeil` and returns `i64`. |
| `isqrtU64` | `(n: u64): u64` | Checks predicate `isqrtU64` and returns `bool`. |
| `icbrtU64` | `(n: u64): u64` | Invokes `icbrtU64` and returns `u64`. |
| `binomialU64` | `(n: u64, k: u64): u64` | Invokes `binomialU64` and returns `u64`. |
| `isEven` | `(n: i64): bool` | Checks predicate `isEven` and returns `bool`. |
| `isOdd` | `(n: i64): bool` | Checks predicate `isOdd` and returns `bool`. |
| `nextMultipleOfU64` | `(n: u64, step: u64): u64` | Invokes `nextMultipleOfU64` and returns `u64`. |
| `prevMultipleOfU64` | `(n: u64, step: u64): u64` | Invokes `prevMultipleOfU64` and returns `u64`. |

## Minimal Example

```cp
import { isqrtU64 } from "num-integer";

pub fn main() {
    let result = isqrtU64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
