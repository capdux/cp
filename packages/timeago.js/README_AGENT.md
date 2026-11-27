# timeago.js — AI Agent Quick Ref

Zero-allocation deterministic relative time formatter (EN/ZH, Long/Short/Compact) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TimeagoLocale,
    TimeagoStyle,
    TimeUnit,
    ScaledDelta,
    absU64,
    appendByte,
    appendStr,
    appendU64,
} from "timeago.js";
```

## Core Types & Constants

```cp
enum TimeagoLocale { En, Zh }
enum TimeagoStyle { Long, Short }
enum TimeUnit { Second, Minute, Hour, Day, Week, Month, ... }
class ScaledDelta { value: u64, unit: TimeUnit }
```

## API Signatures

```cp
fn absU64(n: i64): u64;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn appendStr(out: []u8, pos: usize, s: string): usize;
fn appendU64(out: []u8, pos: usize, val: u64): usize;
fn scaleDelta(abs_sec: u64): ScaledDelta;
fn enLongUnit(unit: TimeUnit, plural: bool): string;
fn enShortUnit(unit: TimeUnit): string;
fn zhLongUnit(unit: TimeUnit): string;
fn zhShortUnit(unit: TimeUnit): string;
fn formatTimeago(delta_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string;
fn formatBetween(target_sec: i64, now_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string;
fn formatTimeagoEn(delta_sec: i64, out: []u8): string;
fn formatTimeagoZh(delta_sec: i64, out: []u8): string;
fn formatCompact(delta_sec: i64, out: []u8): string;
```

## Minimal Usage

```cp
import { appendStr } from "timeago.js";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = appendStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"timeago.js"`; install via `cap add timeago.js`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
