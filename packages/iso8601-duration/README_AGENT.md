# iso8601-duration — AI Agent Quick Ref

Zero-allocation ISO 8601 duration parser, canonical formatter, and millisecond converter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    IsoDuration,
    invalidDuration,
    parseIsoDuration,
    pushByte,
    pushU32,
    pushComp,
    formatIsoDuration,
    toApproxMillis,
} from "iso8601-duration";
```

## Core Types & Constants

```cp
class IsoDuration { negative: bool, years: u32, months: u32, weeks: u32, /* +6 fields */ }
```

## API Signatures

```cp
fn invalidDuration(): IsoDuration;
fn parseIsoDuration(src: &[]u8): IsoDuration;
fn pushByte(dst: []u8, pos: usize, b: u8): i64;
fn pushU32(dst: []u8, pos: usize, val: u32): i64;
fn pushComp(dst: []u8, pos: usize, val: u32, unit: u8): i64;
fn formatIsoDuration(d: IsoDuration, dst: []u8): i64;
fn toApproxMillis(d: IsoDuration): i64;
```

## Minimal Usage

```cp
import { parseIsoDuration } from "iso8601-duration";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseIsoDuration(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"iso8601-duration"`; install via `cap add iso8601-duration`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
