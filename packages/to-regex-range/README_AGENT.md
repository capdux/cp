# to-regex-range — AI Agent Quick Ref

Zero-allocation numeric range to compact regex pattern compiler and numeric range matcher for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TrrBufWriter,
    bwPush,
    countDigits,
    absI64ToU64,
    trrWriteU64,
    isSinglePattern,
    emitSubRange,
    insertStop,
} from "to-regex-range";
```

## Core Types & Constants

```cp
class TrrBufWriter { pos: usize, overflow: bool }
```

## API Signatures

```cp
fn bwPush(w: &mut TrrBufWriter, out: []u8, ch: u8);
fn countDigits(n: u64): usize;
fn absI64ToU64(v: i64): u64;
fn trrWriteU64(w: &mut TrrBufWriter, out: []u8, v: u64);
fn isSinglePattern(a: u64, b: u64): bool;
fn emitSubRange(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64);
fn insertStop(stops: []u64, count: &mut usize, val: u64);
fn sortStops(stops: []u64, count: usize);
fn splitPositiveStops(a: u64, b: u64, stops: []u64): usize;
fn emitPositiveRange(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64);
fn emitNegativeRange(w: &mut TrrBufWriter, out: []u8, mag_lo: u64, mag_hi: u64);
fn toRegexRange(min_val: i64, max_val: i64, out: []u8): usize;
fn matchNumericRange(s: string, min_val: i64, max_val: i64): bool;
```

## Minimal Usage

```cp
import { matchNumericRange } from "to-regex-range";

pub fn main() {
    let result = matchNumericRange("example", 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"to-regex-range"`; install via `cap add to-regex-range`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
