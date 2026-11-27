[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# iso8601

Zero-heap ISO-8601 calendar/week/ordinal date, time, timezone offset, datetime, and duration parser/formatter (crates.io iso8601 equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:iso8601` ([https://github.com/badboy/iso8601](https://github.com/badboy/iso8601))

| Metadata | Value |
| --- | --- |
| Package | `iso8601` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add iso8601
```

```cp
import {
    IsoDate,
    IsoTime,
    IsoDateTime,
    IsoDuration,
    isoIsDigit,
    isoParseDigits,
    isLeapYear,
    daysInMonth,
} from "iso8601";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `IsoDate` | `class` | Fields: `year: i32, month: u8, day: u8, ordinal: u16, week: u8, weekday: u8, +1 more` |
| `IsoTime` | `class` | Fields: `hour: u8, minute: u8, second: u8, millisecond: u16, tz_offset_minutes: i16, has_tz: bool` |
| `IsoDateTime` | `class` | Fields: `date: IsoDate, time: IsoTime` |
| `IsoDuration` | `class` | Fields: `years: u32, months: u32, weeks: u32, days: u32, hours: u32, minutes: u32, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isoIsDigit` | `(c: u8): bool` | Checks predicate `isoIsDigit` and returns `bool`. |
| `isoParseDigits` | `(s: string, start: usize, count: usize): i64?` | Checks predicate `isoParseDigits` and returns `bool`. |
| `isLeapYear` | `(year: i32): bool` | Checks predicate `isLeapYear` and returns `bool`. |
| `daysInMonth` | `(year: i32, month: u8): u8` | Invokes `daysInMonth` and returns `u8`. |
| `parseDate` | `(s: string): IsoDate?` | Parses input via `parseDate` and returns `IsoDate?`. |
| `parseTime` | `(s: string): IsoTime?` | Parses input via `parseTime` and returns `IsoTime?`. |
| `parseDateTime` | `(s: string): IsoDateTime?` | Parses input via `parseDateTime` and returns `IsoDateTime?`. |
| `parseDuration` | `(s: string): IsoDuration?` | Parses input via `parseDuration` and returns `IsoDuration?`. |
| `isoWrite2` | `(out: []u8, pos: usize, val: u8): usize` | Checks predicate `isoWrite2` and returns `bool`. |
| `isoWrite4` | `(out: []u8, pos: usize, val: i32): usize` | Checks predicate `isoWrite4` and returns `bool`. |
| `formatDateTime` | `(dt: IsoDateTime, out: []u8): usize` | Serializes/formats data via `formatDateTime` returning `usize`. |

## Minimal Example

```cp
import { isoParseDigits } from "iso8601";

pub fn main() {
    let result = isoParseDigits("example", 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
