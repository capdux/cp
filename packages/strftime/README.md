[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# strftime

Zero-heap POSIX strftime & ISO-8601 date/time formatter for Unix timestamps and broken-down calendar dates

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strftime` ([https://github.com/samsonjs/strftime](https://github.com/samsonjs/strftime))

| Metadata | Value |
| --- | --- |
| Package | `strftime` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add strftime
```

```cp
import {
    DateTime,
    FmtWriter,
    isLeapYear,
    daysInMonth,
    dayOfYear,
    fromUnixSeconds,
    weekdayShort,
    weekdayFull,
} from "strftime";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DateTime` | `class` | Fields: `year: i32, month: u8, day: u8, hour: u8, minute: u8, second: u8, +3 more` |
| `FmtWriter` | `class` | Fields: `out: []u8, pos: usize` · Methods: `putByte, putStr, putPad2, putPad3, putPad4` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isLeapYear` | `(y: i32): bool` | Checks predicate `isLeapYear` and returns `bool`. |
| `daysInMonth` | `(y: i32, m: u8): u8` | Invokes `daysInMonth` and returns `u8`. |
| `dayOfYear` | `(y: i32, m: u8, d: u8): u16` | Invokes `dayOfYear` and returns `u16`. |
| `fromUnixSeconds` | `(unix_sec: i64, tz_offset_sec: i32): DateTime` | Invokes `fromUnixSeconds` and returns `DateTime`. |
| `weekdayShort` | `(w: u8): string` | Invokes `weekdayShort` and returns `string`. |
| `weekdayFull` | `(w: u8): string` | Invokes `weekdayFull` and returns `string`. |
| `monthShort` | `(m: u8): string` | Invokes `monthShort` and returns `string`. |
| `monthFull` | `(m: u8): string` | Invokes `monthFull` and returns `string`. |
| `strftime` | `(fmt: string, dt: DateTime, out: []u8): string` | Invokes `strftime` and returns `string`. |
| `formatUnix` | `(fmt: string, unix_sec: i64, tz_offset_sec: i32, out: []u8): string` | Serializes/formats data via `formatUnix` returning `string`. |
| `formatIso8601Utc` | `(unix_sec: i64, out: []u8): string` | Serializes/formats data via `formatIso8601Utc` returning `string`. |

## Minimal Example

```cp
import { formatUnix } from "strftime";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatUnix("example", 0, 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
