# rfc3339 — AI Agent Quick Ref

Zero-heap strict RFC 3339 timestamp parser, Unix epoch converter, and UTC formatter (reuses iso8601) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Rfc3339Timestamp,
    rfcIsDigit,
    daysFromCivil,
    civilFromDays,
    isValidRfc3339,
    parseRfc3339,
    fromUnixSeconds,
    formatRfc3339Utc,
} from "rfc3339";
```

## Core Types & Constants

```cp
class Rfc3339Timestamp { dt: IsoDateTime, unix_seconds: i64, millisecond: u16 }
```

## API Signatures

```cp
fn rfcIsDigit(c: u8): bool;
fn daysFromCivil(year: i32, month: u8, day: u8): i64;
fn civilFromDays(z_in: i64): IsoDate;
fn isValidRfc3339(s: string): bool;
fn parseRfc3339(s: string): Rfc3339Timestamp?;
fn fromUnixSeconds(unix_secs: i64): IsoDateTime;
fn formatRfc3339Utc(unix_secs: i64, out: []u8): usize;
fn isRfcLeapYear(year: i32): bool;
```

## Minimal Usage

```cp
import { isValidRfc3339 } from "rfc3339";

pub fn main() {
    let result = isValidRfc3339("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rfc3339"`; install via `cap add rfc3339`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
