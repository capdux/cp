# iso8601@1.0.0 (Agent Quickref)

> Zero-heap ISO-8601 calendar/week/ordinal date, time, timezone offset, datetime, and duration parser/formatter (crates.io iso8601 equivalent) | License: MIT
> Import: `import { ... } from "iso8601";` | Full types: `src/index.d.cp`

## Functions
- `export fn isoIsDigit(c: u8): bool;`
- `export fn isoParseDigits(s: string, start: usize, count: usize): i64?;`
- `export fn isLeapYear(year: i32): bool;`
- `export fn daysInMonth(year: i32, month: u8): u8;`
- `export fn parseDate(s: string): IsoDate?;`
- `export fn parseTime(s: string): IsoTime?;`
- `export fn parseDateTime(s: string): IsoDateTime?;`
- `export fn parseDuration(s: string): IsoDuration?;`
- `export fn isoWrite2(out: []u8, pos: usize, val: u8): usize;`
- `export fn isoWrite4(out: []u8, pos: usize, val: i32): usize;`
- `export fn formatDateTime(dt: IsoDateTime, out: []u8): usize;`
