[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# h3o

Zero-heap Uber H3 64-bit hexagonal hierarchical spatial index cell parser, constructor, hierarchy traversal, and area table (clean-room port of Rust h3o)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:h3o` ([https://github.com/HydroniumLabs/h3o](https://github.com/HydroniumLabs/h3o))

| Metadata | Value |
| --- | --- |
| Package | `h3o` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add h3o
```

```cp
import {
    H3Cell,
    h3AllLow63,
    h3DigitShift,
    h3IsPentagonBaseCell,
    h3HexDigitVal,
    h3HexChar,
    h3ClearBits,
    parseH3Hex,
} from "h3o";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `H3Cell` | `class` | Fields: `bits: u64` · Methods: `fromParts, fromU64, resolution, baseCell, digitAt, isPentagon, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `h3AllLow63` | `(): u64` | Invokes `h3AllLow63` and returns `u64`. |
| `h3DigitShift` | `(r: u8): u6` | Invokes `h3DigitShift` and returns `u6`. |
| `h3IsPentagonBaseCell` | `(bc: u8): bool` | Invokes `h3IsPentagonBaseCell` and returns `bool`. |
| `h3HexDigitVal` | `(c: u8): u8` | Invokes `h3HexDigitVal` and returns `u8`. |
| `h3HexChar` | `(nibble: u8): u8` | Invokes `h3HexChar` and returns `u8`. |
| `h3ClearBits` | `(val: u64, mask: u64): u64` | Invokes `h3ClearBits` and returns `u64`. |
| `parseH3Hex` | `(hex: string): H3Cell?` | Parses input via `parseH3Hex` and returns `H3Cell?`. |
| `avgHexAreaKm2` | `(res: u8): f64` | Invokes `avgHexAreaKm2` and returns `f64`. |

## Minimal Example

```cp
import { parseH3Hex } from "h3o";

pub fn main() {
    let result = parseH3Hex("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
