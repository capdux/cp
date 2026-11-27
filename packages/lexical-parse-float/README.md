[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# lexical-parse-float

Zero-allocation pure CP strict floating-point and radix integer string parser (rust-lexical lexical-parse-float / lexical-parse-integer / lexical-util parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:lexical-parse-float` ([https://github.com/Alexhuszagh/rust-lexical](https://github.com/Alexhuszagh/rust-lexical))

| Metadata | Value |
| --- | --- |
| Package | `lexical-parse-float` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add lexical-parse-float
```

```cp
import {
    ParseFloatResult,
    ParseIntResult,
    lexpToLower,
    lexpEqIgnoreCase,
    lexpDecodeDigit,
    lexpScalePow10,
    parseFloat,
    parseIntRadix,
} from "lexical-parse-float";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParseFloatResult` | `class` | Fields: `ok: bool, value: f64, consumed: usize` |
| `ParseIntResult` | `class` | Fields: `ok: bool, value: i64, consumed: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lexpToLower` | `(c: u8): u8` | Invokes `lexpToLower` and returns `u8`. |
| `lexpEqIgnoreCase` | `(a: string, b: string): bool` | Invokes `lexpEqIgnoreCase` and returns `bool`. |
| `lexpDecodeDigit` | `(c: u8): i32` | Invokes `lexpDecodeDigit` and returns `i32`. |
| `lexpScalePow10` | `(val_in: f64, exp10: i32): f64` | Invokes `lexpScalePow10` and returns `f64`. |
| `parseFloat` | `(s: string): ParseFloatResult` | Parses input via `parseFloat` and returns `ParseFloatResult`. |
| `parseIntRadix` | `(s: string, radix: u32): ParseIntResult` | Parses input via `parseIntRadix` and returns `ParseIntResult`. |

## Minimal Example

```cp
import { parseFloat } from "lexical-parse-float";

pub fn main() {
    let result = parseFloat("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
