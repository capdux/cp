# humantime — AI Agent Quick Ref

Zero-heap human-readable duration parser/formatter and RFC 3339 timestamp codec (clean-room CP implementation of crates.io humantime) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DurationParseResult,
    htIsSpace,
    htIsDigit,
    htIsAlpha,
    htEq,
    htUnitNanos,
    parseDuration,
    htPushStr,
} from "humantime";
```

## Core Types & Constants

```cp
class DurationParseResult { ok: bool, nanos: u64 }
```

## API Signatures

```cp
fn htIsSpace(c: u8): bool;
fn htIsDigit(c: u8): bool;
fn htIsAlpha(c: u8): bool;
fn htEq(s: string, start: usize, end: usize, lit: string): bool;
fn htUnitNanos(s: string, start: usize, end: usize): u64;
fn parseDuration(input: string): DurationParseResult;
fn htPushStr(out: []u8, pos: usize, s: string): usize;
fn htPushPart(out: []u8, pos: usize, val: u64, suffix: string): usize;
fn formatDuration(nanos: u64, out: []u8): usize;
fn htPush2Digits(out: []u8, pos: usize, v: u64): usize;
fn htPush4Digits(out: []u8, pos: usize, v: u64): usize;
fn formatRfc3339Seconds(unix_secs: u64, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseDuration } from "humantime";

pub fn main() {
    let result = parseDuration("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"humantime"`; install via `cap add humantime`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
