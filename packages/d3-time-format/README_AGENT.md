# d3-time-format — AI Agent Quick Ref

Zero-heap UTC strftime/strptime date formatter and parser for CP (clean-room ISC implementation of npm d3-time-format). (`v1.0.0`, `ISC`)

## Import

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

## Core Types & Constants

```cp
class D3tfDateTime { year: u64, month: u64, day: u64, hour: u64, /* +5 fields */ }
class D3tfUintParse { ok: bool, val: u64, next_pos: usize }
```

## API Signatures

```cp
fn d3tfIsLeapYear(y: u64): bool;
fn d3tfDaysInMonth(y: u64, m: u64): u64;
fn d3tfDayOfYear(y: u64, m: u64, d: u64): u64;
fn d3tfFromUnixMs(unix_ms: i64): D3tfDateTime;
fn d3tfToUnixMs(year: u64, month: u64, day: u64, hour: u64, minute: u64, second: u64, ms: u64): i64;
fn d3tfMonthShort(m: u64): string;
fn d3tfMonthFull(m: u64): string;
fn d3tfWeekdayShort(w: u64): string;
fn d3tfWeekdayFull(w: u64): string;
fn d3tfWritePadded(out: []u8, pos: usize, val: u64, width: usize): usize;
fn utcFormat(spec: string, unix_ms: i64, out: []u8): usize;
fn d3tfMatchWordCI(input: string, pos: usize, word: string): bool;
fn d3tfParseDigits(input: string, pos: usize, min_d: usize, max_d: usize): D3tfUintParse;
fn utcParse(spec: string, input: string): i64?;
```

## Minimal Usage

```cp
import { utcFormat } from "d3-time-format";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = utcFormat("example", 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"d3-time-format"`; install via `cap add d3-time-format`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
