[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# http-range-header

Zero-heap RFC 9110 HTTP Range header parser, range validator, and Content-Range formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:http-range-header` ([https://github.com/MarcusGrass/http-range-header](https://github.com/MarcusGrass/http-range-header))

| Metadata | Value |
| --- | --- |
| Package | `http-range-header` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add http-range-header
```

```cp
import {
    RangePosition,
    SyntacticByteRange,
    ValidatedByteRange,
    hrhIsSpace,
    hrhParseU64,
    parseRangeHeader,
    validateRanges,
    formatContentRange,
} from "http-range-header";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `RangePosition` | `StartAndEnd, StartOnly, Suffix` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SyntacticByteRange` | `class` | Fields: `kind: RangePosition, start: u64, end: u64` |
| `ValidatedByteRange` | `class` | Fields: `start: u64, end: u64` · Methods: `byteLen` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hrhIsSpace` | `(b: u8): bool` | Invokes `hrhIsSpace` and returns `bool`. |
| `hrhParseU64` | `(s: string, start: usize, end: usize): u64?` | Invokes `hrhParseU64` and returns `u64?`. |
| `parseRangeHeader` | `(header: string, out: []SyntacticByteRange): usize` | Parses input via `parseRangeHeader` and returns `usize`. |
| `validateRanges` | `(ranges: &[]SyntacticByteRange, total_size: u64, out: []ValidatedByteRange): i64` | Validates input with `validateRanges` returning `i64`. |
| `formatContentRange` | `(start: u64, end: u64, total: u64, out: []u8): usize` | Serializes/formats data via `formatContentRange` returning `usize`. |
| `formatUnsatisfiableContentRange` | `(total: u64, out: []u8): usize` | Serializes/formats data via `formatUnsatisfiableContentRange` returning `usize`. |

## Minimal Example

```cp
import { parseRangeHeader } from "http-range-header";

pub fn main() {
    let result = parseRangeHeader("example", /* out: []SyntacticByteRange */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
