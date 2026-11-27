# strftime — AI Agent Quick Ref

Zero-heap POSIX strftime & ISO-8601 date/time formatter for Unix timestamps and broken-down calendar dates (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class DateTime { year: i32, month: u8, day: u8, hour: u8, /* +5 fields */ }
class FmtWriter { out: []u8, pos: usize } // methods: putByte(...) +4
```

## API Signatures

```cp
fn isLeapYear(y: i32): bool;
fn daysInMonth(y: i32, m: u8): u8;
fn dayOfYear(y: i32, m: u8, d: u8): u16;
fn fromUnixSeconds(unix_sec: i64, tz_offset_sec: i32): DateTime;
fn weekdayShort(w: u8): string;
fn weekdayFull(w: u8): string;
fn monthShort(m: u8): string;
fn monthFull(m: u8): string;
fn strftime(fmt: string, dt: DateTime, out: []u8): string;
fn formatUnix(fmt: string, unix_sec: i64, tz_offset_sec: i32, out: []u8): string;
fn formatIso8601Utc(unix_sec: i64, out: []u8): string;
```

## Minimal Usage

```cp
import { formatUnix } from "strftime";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatUnix("example", 0, 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"strftime"`; install via `cap add strftime`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
