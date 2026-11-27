[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# strnum

Zero-heap configurable string-to-number parser for CP (clean-room MIT implementation of npm strnum).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strnum` ([https://github.com/NaturalIntelligence/strnum](https://github.com/NaturalIntelligence/strnum))（原库名 `strnum` → CP 包名 `strnum`）

| Metadata | Value |
| --- | --- |
| Package | `strnum` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add strnum
```

```cp
import {
    StrNumOptions,
    StrNumResult,
    snumInvalid,
    snumIsSpace,
    snumHexDigit,
    snumScalePow10,
    parseStrNum,
} from "strnum";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StrNumOptions` | `class` | Fields: `hex: bool, leading_zeros: bool, e_notation: bool` |
| `StrNumResult` | `class` | Fields: `is_number: bool, value: f64, is_int: bool, int_value: i64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `snumInvalid` | `(): StrNumResult` | Invokes `snumInvalid` and returns `StrNumResult`. |
| `snumIsSpace` | `(ch: u8): bool` | Invokes `snumIsSpace` and returns `bool`. |
| `snumHexDigit` | `(ch: u8): i32` | Invokes `snumHexDigit` and returns `i32`. |
| `snumScalePow10` | `(val: f64, exp10: i32): f64` | Invokes `snumScalePow10` and returns `f64`. |
| `parseStrNum` | `(input: string, opts: StrNumOptions): StrNumResult` | Parses input via `parseStrNum` and returns `StrNumResult`. |

## Minimal Example

```cp
import { parseStrNum } from "strnum";

pub fn main() {
    let result = parseStrNum("example", /* opts: StrNumOptions */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
