[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# itertools

Pure CP zero-allocation iterator combinators and sequence algorithms (Rust itertools parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:itertools` ([https://github.com/rust-itertools/itertools](https://github.com/rust-itertools/itertools))

| Metadata | Value |
| --- | --- |
| Package | `itertools` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add itertools
```

```cp
import {
    MinMaxResult,
    PartitionResult,
    CartesianPair,
    itCopyBytes,
    minmax,
    isSorted,
    isSortedDesc,
    allEqual,
} from "itertools";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MinMaxResult` | `class` | Fields: `empty: bool, min: i64, max: i64` |
| `PartitionResult` | `class` | Fields: `true_count: usize, false_count: usize` |
| `CartesianPair` | `class` | Fields: `found: bool, first: i64, second: i64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `itCopyBytes` | `(dst: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `itCopyBytes` and returns `bool`. |
| `minmax` | `(slice: &[]i64): MinMaxResult` | Invokes `minmax` and returns `MinMaxResult`. |
| `isSorted` | `(slice: &[]i64): bool` | Checks predicate `isSorted` and returns `bool`. |
| `isSortedDesc` | `(slice: &[]i64): bool` | Checks predicate `isSortedDesc` and returns `bool`. |
| `allEqual` | `(slice: &[]i64): bool` | Invokes `allEqual` and returns `bool`. |
| `uniqueI64` | `(src: &[]i64, out: []i64): usize` | Invokes `uniqueI64` and returns `usize`. |
| `dedupConsecutive` | `(src: &[]i64, out: []i64): usize` | Invokes `dedupConsecutive` and returns `usize`. |
| `interleave` | `(a: &[]i64, b: &[]i64, out: []i64): usize` | Invokes `interleave` and returns `usize`. |
| `intersperse` | `(src: &[]i64, sep: i64, out: []i64): usize` | Invokes `intersperse` and returns `usize`. |
| `mergeSorted` | `(a: &[]i64, b: &[]i64, out: []i64): usize` | Invokes `mergeSorted` and returns `usize`. |
| `joinStrings` | `(items: &[]string, sep: string, out: []u8): usize` | Invokes `joinStrings` and returns `usize`. |
| `partitionByThreshold` | `(src: &[]i64, threshold: i64, out_true: []i64, out_false: []i64): PartitionResult` | Invokes `partitionByThreshold` and returns `PartitionResult`. |
| `cartesianProductCount` | `(a_len: usize, b_len: usize): usize` | Invokes `cartesianProductCount` and returns `usize`. |
| `nthCartesianPair` | `(a: &[]i64, b: &[]i64, idx: usize): CartesianPair` | Invokes `nthCartesianPair` and returns `CartesianPair`. |
| `chunkCount` | `(total_len: usize, chunk_size: usize): usize` | Invokes `chunkCount` and returns `usize`. |
| `windowCount` | `(total_len: usize, win_size: usize): usize` | Invokes `windowCount` and returns `usize`. |

## Minimal Example

```cp
import { joinStrings } from "itertools";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = joinStrings(/* items: &[]string */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
