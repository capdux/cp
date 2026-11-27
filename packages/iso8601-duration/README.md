[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# iso8601-duration

Zero-allocation ISO 8601 duration parser, canonical formatter, and millisecond converter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:iso8601-duration` ([https://github.com/tolu/ISO8601-duration](https://github.com/tolu/ISO8601-duration))（原库名 `iso8601-duration` → CP 包名 `iso8601-duration`）

| Metadata | Value |
| --- | --- |
| Package | `iso8601-duration` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add iso8601-duration
```

```cp
import {
    IsoDuration,
    invalidDuration,
    parseIsoDuration,
    pushByte,
    pushU32,
    pushComp,
    formatIsoDuration,
    toApproxMillis,
} from "iso8601-duration";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `IsoDuration` | `class` | Fields: `negative: bool, years: u32, months: u32, weeks: u32, days: u32, hours: u32, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidDuration` | `(): IsoDuration` | Invokes `invalidDuration` and returns `IsoDuration`. |
| `parseIsoDuration` | `(src: &[]u8): IsoDuration` | Parses input via `parseIsoDuration` and returns `IsoDuration`. |
| `pushByte` | `(dst: []u8, pos: usize, b: u8): i64` | Mutates or appends state via `pushByte`. |
| `pushU32` | `(dst: []u8, pos: usize, val: u32): i64` | Mutates or appends state via `pushU32`. |
| `pushComp` | `(dst: []u8, pos: usize, val: u32, unit: u8): i64` | Mutates or appends state via `pushComp`. |
| `formatIsoDuration` | `(d: IsoDuration, dst: []u8): i64` | Serializes/formats data via `formatIsoDuration` returning `i64`. |
| `toApproxMillis` | `(d: IsoDuration): i64` | Invokes `toApproxMillis` and returns `i64`. |

## Minimal Example

```cp
import { parseIsoDuration } from "iso8601-duration";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseIsoDuration(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
