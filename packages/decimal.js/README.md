[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# decimal.js

Zero-allocation 128-bit exact decimal & financial arithmetic engine (decimal.js / bignumber.js / rust_decimal parity: exact add/sub/mul/div, 6 rounding modes including Banker's HalfEven, scientific notation parsing) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:decimal.js` ([https://github.com/MikeMcl/decimal.js](https://github.com/MikeMcl/decimal.js))（原库名 `decimal.js` → CP 包名 `decimal.js`）

| Metadata | Value |
| --- | --- |
| Package | `decimal.js` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add decimal.js
```

```cp
import {
    RoundingMode,
    Decimal,
    pow10,
    absI128,
    divRoundI128,
} from "decimal.js";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `RoundingMode` | `HalfEven, HalfUp, HalfDown, Floor, Ceil, Trunc` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Decimal` | `class` | Fields: `mantissa: i128, scale: u8` · Methods: `zero, one, fromInt, fromParts, parse, isZero, +21 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pow10` | `(exp: u8): i128` | Invokes `pow10` and returns `i128`. |
| `absI128` | `(v: i128): i128` | Invokes `absI128` and returns `i128`. |
| `divRoundI128` | `(num: i128, den: i128, mode: RoundingMode): i128` | Invokes `divRoundI128` and returns `i128`. |

## Minimal Example

```cp
import { pow10 } from "decimal.js";

pub fn main() {
    let result = pow10(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
