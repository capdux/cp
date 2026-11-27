[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bytesize

Zero-heap integer byte size constants (SI & IEC), ByteSize builder, parser, and human-readable formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bytesize` ([https://github.com/hyunsik/bytesize](https://github.com/hyunsik/bytesize))

| Metadata | Value |
| --- | --- |
| Package | `bytesize` |
| Version | `1.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bytesize
```

```cp
import {
    ByteSize,
    bszWriteByte,
    bszUnitMultiplier,
    bszFormatScaled,
    formatSi,
    formatIec,
    parseByteSize,
} from "bytesize";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ByteSize` | `class` | Fields: `val: u64` · Methods: `b, kb, mb, gb, tb, pb, +10 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bszWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `bszWriteByte` and returns `usize`. |
| `bszUnitMultiplier` | `(s: string): u64?` | Invokes `bszUnitMultiplier` and returns `u64?`. |
| `bszFormatScaled` | `(bytes: u64, unit: u64, suffix: string, out: []u8): usize` | Invokes `bszFormatScaled` and returns `usize`. |
| `formatSi` | `(bytes: u64, out: []u8): usize` | Serializes/formats data via `formatSi` returning `usize`. |
| `formatIec` | `(bytes: u64, out: []u8): usize` | Serializes/formats data via `formatIec` returning `usize`. |
| `parseByteSize` | `(input: string): u64?` | Parses input via `parseByteSize` and returns `u64?`. |

## Minimal Example

```cp
import { parseByteSize } from "bytesize";

pub fn main() {
    let result = parseByteSize("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
