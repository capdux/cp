[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# d3-time-format

Zero-heap UTC strftime/strptime date formatter and parser for CP (clean-room ISC implementation of npm d3-time-format).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-time-format` ([https://github.com/d3/d3-time-format](https://github.com/d3/d3-time-format))（原库名 `d3-time-format` → CP 包名 `d3-time-format`）

| Metadata | Value |
| --- | --- |
| Package | `d3-time-format` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add d3-time-format
```

```cp
import {
    D3tfDateTime,
    D3tfUintParse,
    d3tfIsLeapYear,
    d3tfDaysInMonth,
    d3tfDayOfYear,
    d3tfFromUnixMs,
    d3tfToUnixMs,
    d3tfMonthShort,
} from "d3-time-format";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `D3tfDateTime` | `class` | Fields: `year: u64, month: u64, day: u64, hour: u64, minute: u64, second: u64, +3 more` |
| `D3tfUintParse` | `class` | Fields: `ok: bool, val: u64, next_pos: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `d3tfIsLeapYear` | `(y: u64): bool` | Invokes `d3tfIsLeapYear` and returns `bool`. |
| `d3tfDaysInMonth` | `(y: u64, m: u64): u64` | Invokes `d3tfDaysInMonth` and returns `u64`. |
| `d3tfDayOfYear` | `(y: u64, m: u64, d: u64): u64` | Invokes `d3tfDayOfYear` and returns `u64`. |
| `d3tfFromUnixMs` | `(unix_ms: i64): D3tfDateTime` | Invokes `d3tfFromUnixMs` and returns `D3tfDateTime`. |
| `d3tfToUnixMs` | `(year: u64, month: u64, day: u64, hour: u64, minute: u64, second: u64, ms: u64): i64` | Invokes `d3tfToUnixMs` and returns `i64`. |
| `d3tfMonthShort` | `(m: u64): string` | Invokes `d3tfMonthShort` and returns `string`. |
| `d3tfMonthFull` | `(m: u64): string` | Invokes `d3tfMonthFull` and returns `string`. |
| `d3tfWeekdayShort` | `(w: u64): string` | Invokes `d3tfWeekdayShort` and returns `string`. |
| `d3tfWeekdayFull` | `(w: u64): string` | Invokes `d3tfWeekdayFull` and returns `string`. |
| `d3tfWritePadded` | `(out: []u8, pos: usize, val: u64, width: usize): usize` | Invokes `d3tfWritePadded` and returns `usize`. |
| `utcFormat` | `(spec: string, unix_ms: i64, out: []u8): usize` | Invokes `utcFormat` and returns `usize`. |
| `d3tfMatchWordCI` | `(input: string, pos: usize, word: string): bool` | Invokes `d3tfMatchWordCI` and returns `bool`. |
| `d3tfParseDigits` | `(input: string, pos: usize, min_d: usize, max_d: usize): D3tfUintParse` | Invokes `d3tfParseDigits` and returns `D3tfUintParse`. |
| `utcParse` | `(spec: string, input: string): i64?` | Invokes `utcParse` and returns `i64?`. |

## Minimal Example

```cp
import { utcFormat } from "d3-time-format";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = utcFormat("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
