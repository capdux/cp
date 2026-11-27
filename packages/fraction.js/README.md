[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fraction.js

Zero-heap exact rational number (fraction) arithmetic in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fraction.js` ([https://github.com/rawify/Fraction.js](https://github.com/rawify/Fraction.js)) / `crates.io:fraction` ([https://github.com/dnsl48/fraction](https://github.com/dnsl48/fraction))（原库名 `fraction.js` → CP 包名 `fraction.js`）

| Metadata | Value |
| --- | --- |
| Package | `fraction.js` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fraction.js
```

```cp
import {
    Rational,
    absToU64,
    divTrunc64,
    gcd64,
    zero,
    one,
    fromInt,
    new,
} from "fraction.js";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Rational` | `class` | Fields: `numer: i64, denom: i64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absToU64` | `(v: i64): u64` | Invokes `absToU64` and returns `u64`. |
| `divTrunc64` | `(a: i64, b: i64): i64` | Invokes `divTrunc64` and returns `i64`. |
| `gcd64` | `(a: i64, b: i64): i64` | Invokes `gcd64` and returns `i64`. |
| `zero` | `(): Rational` | Invokes `zero` and returns `Rational`. |
| `one` | `(): Rational` | Invokes `one` and returns `Rational`. |
| `fromInt` | `(n: i64): Rational` | Invokes `fromInt` and returns `Rational`. |
| `new` | `(n: i64, d: i64): Rational` | Constructs/initializes an instance via `new`. |
| `neg` | `(a: Rational): Rational` | Invokes `neg` and returns `Rational`. |
| `abs` | `(a: Rational): Rational` | Invokes `abs` and returns `Rational`. |
| `recip` | `(a: Rational): Rational` | Invokes `recip` and returns `Rational`. |
| `add` | `(a: Rational, b: Rational): Rational` | Mutates or appends state via `add`. |
| `sub` | `(a: Rational, b: Rational): Rational` | Invokes `sub` and returns `Rational`. |
| `mul` | `(a: Rational, b: Rational): Rational` | Invokes `mul` and returns `Rational`. |
| `div` | `(a: Rational, b: Rational): Rational` | Invokes `div` and returns `Rational`. |
| `eq` | `(a: Rational, b: Rational): bool` | Invokes `eq` and returns `bool`. |
| `cmp` | `(a: Rational, b: Rational): i32` | Invokes `cmp` and returns `i32`. |
| `toF64` | `(a: Rational): f64` | Invokes `toF64` and returns `f64`. |
| `floorInt` | `(a: Rational): i64` | Invokes `floorInt` and returns `i64`. |
| `ceilInt` | `(a: Rational): i64` | Invokes `ceilInt` and returns `i64`. |
| `isInteger` | `(a: Rational): bool` | Checks predicate `isInteger` and returns `bool`. |

## Minimal Example

```cp
import { isInteger } from "fraction.js";

pub fn main() {
    let result = isInteger(/* a: Rational */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
