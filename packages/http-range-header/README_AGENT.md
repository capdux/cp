# http-range-header — AI Agent Quick Ref

Zero-heap RFC 9110 HTTP Range header parser, range validator, and Content-Range formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RangePosition,
    SyntacticByteRange,
    ValidatedByteRange,
    hrhIsSpace,
    hrhParseU64,
    parseRangeHeader,
    validateRanges,
    formatContentRange,
} from "http-range-header";
```

## Core Types & Constants

```cp
enum RangePosition { StartAndEnd, StartOnly, Suffix }
class SyntacticByteRange { kind: RangePosition, start: u64, end: u64 }
class ValidatedByteRange { start: u64, end: u64 } // methods: byteLen(...)
```

## API Signatures

```cp
fn hrhIsSpace(b: u8): bool;
fn hrhParseU64(s: string, start: usize, end: usize): u64?;
fn parseRangeHeader(header: string, out: []SyntacticByteRange): usize;
fn validateRanges(ranges: &[]SyntacticByteRange, total_size: u64, out: []ValidatedByteRange): i64;
fn formatContentRange(start: u64, end: u64, total: u64, out: []u8): usize;
fn formatUnsatisfiableContentRange(total: u64, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseRangeHeader } from "http-range-header";

pub fn main() {
    let result = parseRangeHeader("example", /* out: []SyntacticByteRange */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"http-range-header"`; install via `cap add http-range-header`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
