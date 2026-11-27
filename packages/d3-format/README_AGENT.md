# d3-format — AI Agent Quick Ref

Zero-heap D3/Python-style number format specifier parser and formatter for CP (clean-room ISC implementation of npm d3-format). (`v1.0.0`, `ISC`)

## Import

```cp
import {
    FormatSpec,
    d3fIsAlign,
    d3fIsType,
    parseFormatSpec,
    d3fPow10F64,
    d3fPow10U64,
    d3fWriteU64Base,
    d3fWriteFixed,
} from "d3-format";
```

## Core Types & Constants

```cp
class FormatSpec { valid: bool, fill: u8, @"align": u8, sign: u8, /* +7 fields */ }
```

## API Signatures

```cp
fn d3fIsAlign(ch: u8): bool;
fn d3fIsType(ch: u8): bool;
fn parseFormatSpec(spec: string): FormatSpec;
fn d3fPow10F64(exp: i32): f64;
fn d3fPow10U64(exp: i32): u64;
fn d3fWriteU64Base(val: u64, base: u64, upper: bool, comma: bool, out: []u8): usize;
fn d3fWriteFixed(abs_val: f64, prec_in: i32, comma: bool, trim: bool, out: []u8): usize;
fn d3fAppendSiSuffix(exp3: i32, out: []u8, pos: usize): usize;
fn d3fDisplayLen(slice: &[]u8): usize;
fn d3fCopySlice(dst: []u8, pos: usize, src: &[]u8): usize;
fn formatNumber(spec: string, value: f64, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseFormatSpec } from "d3-format";

pub fn main() {
    let result = parseFormatSpec("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"d3-format"`; install via `cap add d3-format`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
