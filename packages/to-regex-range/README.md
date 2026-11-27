[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# to-regex-range

Zero-allocation numeric range to compact regex pattern compiler and numeric range matcher for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:to-regex-range` ([https://github.com/micromatch/to-regex-range](https://github.com/micromatch/to-regex-range))

| Metadata | Value |
| --- | --- |
| Package | `to-regex-range` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add to-regex-range
```

```cp
import {
    TrrBufWriter,
    bwPush,
    countDigits,
    absI64ToU64,
    trrWriteU64,
    isSinglePattern,
    emitSubRange,
    insertStop,
} from "to-regex-range";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TrrBufWriter` | `class` | Fields: `pos: usize, overflow: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bwPush` | `(w: &mut TrrBufWriter, out: []u8, ch: u8)` | Executes `bwPush` in-place. |
| `countDigits` | `(n: u64): usize` | Invokes `countDigits` and returns `usize`. |
| `absI64ToU64` | `(v: i64): u64` | Invokes `absI64ToU64` and returns `u64`. |
| `trrWriteU64` | `(w: &mut TrrBufWriter, out: []u8, v: u64)` | Executes `trrWriteU64` in-place. |
| `isSinglePattern` | `(a: u64, b: u64): bool` | Checks predicate `isSinglePattern` and returns `bool`. |
| `emitSubRange` | `(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64)` | Executes `emitSubRange` in-place. |
| `insertStop` | `(stops: []u64, count: &mut usize, val: u64)` | Executes `insertStop` in-place. |
| `sortStops` | `(stops: []u64, count: usize)` | Executes `sortStops` in-place. |
| `splitPositiveStops` | `(a: u64, b: u64, stops: []u64): usize` | Invokes `splitPositiveStops` and returns `usize`. |
| `emitPositiveRange` | `(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64)` | Executes `emitPositiveRange` in-place. |
| `emitNegativeRange` | `(w: &mut TrrBufWriter, out: []u8, mag_lo: u64, mag_hi: u64)` | Executes `emitNegativeRange` in-place. |
| `toRegexRange` | `(min_val: i64, max_val: i64, out: []u8): usize` | Invokes `toRegexRange` and returns `usize`. |
| `matchNumericRange` | `(s: string, min_val: i64, max_val: i64): bool` | Invokes `matchNumericRange` and returns `bool`. |

## Minimal Example

```cp
import { matchNumericRange } from "to-regex-range";

pub fn main() {
    let result = matchNumericRange("example", 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
