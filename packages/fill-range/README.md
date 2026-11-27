[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fill-range

Zero-heap numeric, alphabetic, stepped, and zero-padded range expansion engine (npm fill-range parity).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fill-range` ([https://github.com/jonschlinkert/fill-range](https://github.com/jonschlinkert/fill-range))

| Metadata | Value |
| --- | --- |
| Package | `fill-range` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fill-range
```

```cp
import {
    ParsedInt,
    isAsciiAlpha,
    absStepU64,
    absDiffI64,
    countRange,
    expandNumbers,
    expandChars,
    parseRangeInt,
} from "fill-range";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedInt` | `class` | Fields: `value: i64, pad_width: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isAsciiAlpha` | `(c: u8): bool` | Checks predicate `isAsciiAlpha` and returns `bool`. |
| `absStepU64` | `(step: i64): u64` | Invokes `absStepU64` and returns `u64`. |
| `absDiffI64` | `(a: i64, b: i64): u64` | Invokes `absDiffI64` and returns `u64`. |
| `countRange` | `(start: i64, end: i64, step: i64): usize` | Invokes `countRange` and returns `usize`. |
| `expandNumbers` | `(start: i64, end: i64, step: i64, out: []i64): i64` | Invokes `expandNumbers` and returns `i64`. |
| `expandChars` | `(start: u8, end: u8, step: usize, out: []u8): i64` | Invokes `expandChars` and returns `i64`. |
| `parseRangeInt` | `(s: &[]u8): ParsedInt` | Parses input via `parseRangeInt` and returns `ParsedInt`. |
| `writeSlice` | `(dst: []u8, pos: usize, src: &[]u8): i64` | Invokes `writeSlice` and returns `i64`. |
| `writePaddedInt` | `(dst: []u8, start_pos: usize, val: i64, pad_width: usize): i64` | Invokes `writePaddedInt` and returns `i64`. |
| `formatRange` | `(start: &[]u8, end: &[]u8, step: i64, sep: &[]u8, dst: []u8): i64` | Serializes/formats data via `formatRange` returning `i64`. |

## Minimal Example

```cp
import { isAsciiAlpha } from "fill-range";

pub fn main() {
    let result = isAsciiAlpha(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
