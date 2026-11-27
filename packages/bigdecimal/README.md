[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bigdecimal

Zero-heap exact base-10 financial arithmetic library with 18-digit precision, rounding, parsing, and formatting (clean-room port of Rust bigdecimal)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bigdecimal` ([https://github.com/akubera/bigdecimal-rs](https://github.com/akubera/bigdecimal-rs))

| Metadata | Value |
| --- | --- |
| Package | `bigdecimal` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bigdecimal
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BigDecimal` | `class` | Fields: `digits: i64, scale: i32` · Methods: `zero, fromInt, new, normalize, neg, abs, +7 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bdecIsSpace` | `(c: u8): bool` | Invokes `bdecIsSpace` and returns `bool`. |
| `bdecIsDigit` | `(c: u8): bool` | Invokes `bdecIsDigit` and returns `bool`. |
| `bdecAbsU64` | `(v: i64): u64` | Invokes `bdecAbsU64` and returns `u64`. |
| `bdecSignedFromU64` | `(mag: u64, neg: bool): i64` | Invokes `bdecSignedFromU64` and returns `i64`. |
| `bdecPow10U64` | `(exp: u32): u64` | Invokes `bdecPow10U64` and returns `u64`. |
| `bdecDigitCount` | `(u: u64): i32` | Invokes `bdecDigitCount` and returns `i32`. |
| `bdecWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `bdecWriteByte` and returns `usize`. |
| `parseDecimal` | `(s: string): BigDecimal?` | Parses input via `parseDecimal` and returns `BigDecimal?`. |
| `formatDecimal` | `(val: BigDecimal, out: []u8): usize` | Serializes/formats data via `formatDecimal` returning `usize`. |

## Minimal Example

```cp
import { parseDecimal } from "bigdecimal";

pub fn main() {
    let result = parseDecimal("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
