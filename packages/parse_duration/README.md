[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# parse_duration

Zero-heap human-readable duration parser and formatter supporting compound expressions, fractional units, Unicode microseconds, and overflow protection (clean-room port of Rust parse_duration / duration-str)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:parse_duration` ([https://github.com/zeta12ti/parse_duration](https://github.com/zeta12ti/parse_duration))（原库名 `parse_duration` → CP 包名 `parse_duration`）

| Metadata | Value |
| --- | --- |
| Package | `parse_duration` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add parse_duration
```

```cp
import {
    ParsedDuration,
    pdurCheckedAddU64,
    pdurCheckedMulU64,
    pdurMatchUnit,
    pdurAppendPart,
    parseDuration,
    formatDuration,
} from "parse_duration";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedDuration` | `class` | Fields: `secs: u64, nanos: u32` · Methods: `isZero, totalSecs, totalMillis, totalNanos` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pdurCheckedAddU64` | `(a: u64, b: u64): u64?` | Invokes `pdurCheckedAddU64` and returns `u64?`. |
| `pdurCheckedMulU64` | `(a: u64, b: u64): u64?` | Invokes `pdurCheckedMulU64` and returns `u64?`. |
| `pdurMatchUnit` | `(u: string): u32` | Invokes `pdurMatchUnit` and returns `u32`. |
| `pdurAppendPart` | `(out: []u8, pos: usize, val: u64, suffix: string): usize` | Invokes `pdurAppendPart` and returns `usize`. |
| `parseDuration` | `(input: string): ParsedDuration?` | Parses input via `parseDuration` and returns `ParsedDuration?`. |
| `formatDuration` | `(d: ParsedDuration, out: []u8): usize` | Serializes/formats data via `formatDuration` returning `usize`. |

## Minimal Example

```cp
import { parseDuration } from "parse_duration";

pub fn main() {
    let result = parseDuration("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
