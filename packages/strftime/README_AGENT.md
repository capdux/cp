# strftime@1.0.0 (Agent Quickref)

> Zero-heap POSIX strftime & ISO-8601 date/time formatter for Unix timestamps and broken-down calendar dates | License: MIT
> Import: `import { ... } from "strftime";` | Full types: `src/index.d.cp`

## Functions
- `export fn isLeapYear(y: i32): bool;`
- `export fn daysInMonth(y: i32, m: u8): u8;`
- `export fn dayOfYear(y: i32, m: u8, d: u8): u16;`
- `export fn fromUnixSeconds(unix_sec: i64, tz_offset_sec: i32): DateTime;`
- `export fn weekdayShort(w: u8): string;`
- `export fn weekdayFull(w: u8): string;`
- `export fn monthShort(m: u8): string;`
- `export fn monthFull(m: u8): string;`
- `export fn strftime(fmt: string, dt: DateTime, out: []u8): string;`
- `export fn formatUnix(fmt: string, unix_sec: i64, tz_offset_sec: i32, out: []u8): string;`
- `export fn formatIso8601Utc(unix_sec: i64, out: []u8): string;`
