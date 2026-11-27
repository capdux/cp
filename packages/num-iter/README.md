[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# num-iter

Zero-heap stepped numeric range iterators for signed and unsigned 64-bit integers with overflow-safe inclusive/exclusive bounds (clean-room port of Rust num-iter)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-iter` ([https://github.com/rust-num/num-iter](https://github.com/rust-num/num-iter))

| Metadata | Value |
| --- | --- |
| Package | `num-iter` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add num-iter
```

```cp
import {
    RangeStep,
    RangeStepInclusive,
    RangeStepU64,
    nitrAbsI64,
    nitrCheckedAddI64,
    nitrDiffU64,
    nitrAdvanceSigned,
    rangeStep,
} from "num-iter";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RangeStep` | `class` | Fields: `state: i64, stop: i64, step: i64, done: bool` · Methods: `isDone, remaining, next, nth` |
| `RangeStepInclusive` | `class` | Fields: `state: i64, stop: i64, step: i64, done: bool` · Methods: `isDone, remaining, next` |
| `RangeStepU64` | `class` | Fields: `state: u64, stop: u64, step: u64, done: bool` · Methods: `isDone, remaining, next, nth` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nitrAbsI64` | `(v: i64): u64` | Invokes `nitrAbsI64` and returns `u64`. |
| `nitrCheckedAddI64` | `(a: i64, b: i64): i64?` | Invokes `nitrCheckedAddI64` and returns `i64?`. |
| `nitrDiffU64` | `(hi: i64, lo: i64): u64` | Invokes `nitrDiffU64` and returns `u64`. |
| `nitrAdvanceSigned` | `(state: i64, step: i64, n: usize): i64` | Invokes `nitrAdvanceSigned` and returns `i64`. |
| `rangeStep` | `(start: i64, stop: i64, step: i64): RangeStep` | Invokes `rangeStep` and returns `RangeStep`. |
| `rangeStepInclusive` | `(start: i64, stop: i64, step: i64): RangeStepInclusive` | Invokes `rangeStepInclusive` and returns `RangeStepInclusive`. |
| `rangeStepU64` | `(start: u64, stop: u64, step: u64): RangeStepU64` | Invokes `rangeStepU64` and returns `RangeStepU64`. |
| `collectRangeStep` | `(r: &mut RangeStep, out: []i64): usize` | Invokes `collectRangeStep` and returns `usize`. |

## Minimal Example

```cp
import { nitrAbsI64 } from "num-iter";

pub fn main() {
    let result = nitrAbsI64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
