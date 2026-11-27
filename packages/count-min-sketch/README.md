[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# count-min-sketch

Zero-allocation Count-Min Sketch streaming frequency estimator and heavy-hitter detector (Rust count-min-sketch / Redis CMS parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:count-min-sketch` ([https://github.com/amansarda/count-min-sketch](https://github.com/amansarda/count-min-sketch))（原库名 `count-min-sketch` → CP 包名 `count-min-sketch`）

| Metadata | Value |
| --- | --- |
| Package | `count-min-sketch` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add count-min-sketch
```

```cp
import {
    CountMinSketch,
    makeU64,
    addWrap64,
    mulWrap64,
    splitMix64,
    rowSeed,
    hashBytesRow,
    hashU64Row,
} from "count-min-sketch";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CountMinSketch` | `class` | Fields: `counters: [CMS_CELLS]u32, total: u64` · Methods: `init, clear, width, depth, totalCount, add, +6 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | Invokes `makeU64` and returns `u64`. |
| `addWrap64` | `(a: u64, b: u64): u64` | Mutates or appends state via `addWrap64`. |
| `mulWrap64` | `(a: u64, b: u64): u64` | Invokes `mulWrap64` and returns `u64`. |
| `splitMix64` | `(x_in: u64): u64` | Invokes `splitMix64` and returns `u64`. |
| `rowSeed` | `(row: usize): u64` | Invokes `rowSeed` and returns `u64`. |
| `hashBytesRow` | `(s: string, row: usize): usize` | Checks predicate `hashBytesRow` and returns `bool`. |
| `hashU64Row` | `(val: u64, row: usize): usize` | Checks predicate `hashU64Row` and returns `bool`. |
| `satAddU32` | `(a: u32, b: u32): u32` | Invokes `satAddU32` and returns `u32`. |

## Minimal Example

```cp
import { hashBytesRow } from "count-min-sketch";

pub fn main() {
    let result = hashBytesRow("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
