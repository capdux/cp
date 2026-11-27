# iso8601 — AI Agent Quick Ref

Zero-heap ISO-8601 calendar/week/ordinal date, time, timezone offset, datetime, and duration parser/formatter (crates.io iso8601 equivalent) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class IsoDate { year: i32, month: u8, day: u8, ordinal: u16, /* +3 fields */ }
class IsoTime { hour: u8, minute: u8, second: u8, millisecond: u16, /* +2 fields */ }
class IsoDateTime { date: IsoDate, time: IsoTime }
class IsoDuration { years: u32, months: u32, weeks: u32, days: u32, /* +4 fields */ }
```

## API Signatures

```cp
fn isoIsDigit(c: u8): bool;
fn isoParseDigits(s: string, start: usize, count: usize): i64?;
fn isLeapYear(year: i32): bool;
fn daysInMonth(year: i32, month: u8): u8;
fn parseDate(s: string): IsoDate?;
fn parseTime(s: string): IsoTime?;
fn parseDateTime(s: string): IsoDateTime?;
fn parseDuration(s: string): IsoDuration?;
fn isoWrite2(out: []u8, pos: usize, val: u8): usize;
fn isoWrite4(out: []u8, pos: usize, val: i32): usize;
fn formatDateTime(dt: IsoDateTime, out: []u8): usize;
```

## Minimal Usage

```cp
import { isoParseDigits } from "iso8601";

pub fn main() {
    let result = isoParseDigits("example", 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"iso8601"`; install via `cap add iso8601`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
