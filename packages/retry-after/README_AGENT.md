# retry-after — AI Agent Quick Ref

Zero-heap RFC 9110 HTTP Retry-After header parser and formatter (delay-seconds & HTTP-date) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RetryAfter,
    parseUint,
    parseMonth,
    isLeapYear,
    daysInMonth,
    daysFromCivil,
    parseHttpDate,
    parseRetryAfter,
} from "retry-after";
```

## Core Types & Constants

```cp
class RetryAfter { kind: u8, delay_seconds: u64, epoch_seconds: i64, valid: bool }
```

## API Signatures

```cp
fn parseUint(s: &[]u8): i64;
fn parseMonth(s: &[]u8): i32;
fn isLeapYear(y: i64): bool;
fn daysInMonth(y: i64, m: i32): i32;
fn daysFromCivil(y_in: i64, m_in: i64, d: i64): i64;
fn parseHttpDate(s: &[]u8): i64;
fn parseRetryAfter(header: &[]u8): RetryAfter;
fn waitSeconds(ra: RetryAfter, now_epoch_sec: i64): i64;
fn formatDelaySeconds(seconds: u64, dst: []u8): i64;
```

## Minimal Usage

```cp
import { parseUint } from "retry-after";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseUint(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"retry-after"`; install via `cap add retry-after`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
