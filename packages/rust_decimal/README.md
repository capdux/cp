[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rust_decimal

Zero-heap fixed-point financial decimal arithmetic (scale 0..12) with banker's rounding in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rust_decimal` ([https://github.com/paupino/rust-decimal](https://github.com/paupino/rust-decimal))（原库名 `rust_decimal` → CP 包名 `rust_decimal`）

| Metadata | Value |
| --- | --- |
| Package | `rust_decimal` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rust_decimal
```

```cp
import {
    ParseDecResult,
    Decimal,
    absToU64,
    clampScale,
    pow10F64,
} from "rust_decimal";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParseDecResult` | `class` | Fields: `value: Decimal, valid: bool` |
| `Decimal` | `class` | Fields: `mantissa: i64, scale: u8` · Methods: `new, fromInt, zero, one, parse, normalize, +12 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absToU64` | `(v: i64): u64` | Invokes `absToU64` and returns `u64`. |
| `clampScale` | `(s: u8): u8` | Invokes `clampScale` and returns `u8`. |
| `pow10F64` | `(s: u8): f64` | Invokes `pow10F64` and returns `f64`. |

## Minimal Example

```cp
import { absToU64 } from "rust_decimal";

pub fn main() {
    let result = absToU64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
