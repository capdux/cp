# fill-range — AI Agent Quick Ref

Zero-heap numeric, alphabetic, stepped, and zero-padded range expansion engine (npm fill-range parity). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParsedInt,
    isAsciiAlpha,
    absStepU64,
    absDiffI64,
    countRange,
    expandNumbers,
    expandChars,
    parseRangeInt,
} from "fill-range";
```

## Core Types & Constants

```cp
class ParsedInt { value: i64, pad_width: usize, ok: bool }
```

## API Signatures

```cp
fn isAsciiAlpha(c: u8): bool;
fn absStepU64(step: i64): u64;
fn absDiffI64(a: i64, b: i64): u64;
fn countRange(start: i64, end: i64, step: i64): usize;
fn expandNumbers(start: i64, end: i64, step: i64, out: []i64): i64;
fn expandChars(start: u8, end: u8, step: usize, out: []u8): i64;
fn parseRangeInt(s: &[]u8): ParsedInt;
fn writeSlice(dst: []u8, pos: usize, src: &[]u8): i64;
fn writePaddedInt(dst: []u8, start_pos: usize, val: i64, pad_width: usize): i64;
fn formatRange(start: &[]u8, end: &[]u8, step: i64, sep: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { isAsciiAlpha } from "fill-range";

pub fn main() {
    let result = isAsciiAlpha(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fill-range"`; install via `cap add fill-range`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
