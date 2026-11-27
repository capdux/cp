# sprintf-js — AI Agent Quick Ref

Zero-allocation C-style sprintf formatting engine supporting %s/%d/%i/%u/%x/%X/%o/%b/%f/%c with flags, width, and precision in pure CP (`v1.0.0`, `BSD-3-Clause`)

## Import

```cp
import {
    FmtArgKind,
    FmtArg,
    spfPutByte,
    spfPutRepeat,
    spfEmitPadded,
    spfFormatUintRadix,
    spfArgAsSigned,
    spfArgAsUnsigned,
} from "sprintf-js";
```

## Core Types & Constants

```cp
enum FmtArgKind { Int, Uint, Float, Str, Char, Bool }
class FmtArg { kind: FmtArgKind, int_val: i64, uint_val: u64, float_val: f64, /* +3 fields */ } // methods: int(...) +5
```

## API Signatures

```cp
fn spfPutByte(out: []u8, pos: usize, b: u8): usize;
fn spfPutRepeat(out: []u8, pos: usize, b: u8, count: usize): usize;
fn spfEmitPadded(out: []u8, pos: usize, sign: u8, body: &[]u8, width: usize, left_align: bool, zero_pad: bool): usize;
fn spfFormatUintRadix(val: u64, base: u64, upper: bool, buf: []u8): usize;
fn spfArgAsSigned(arg: FmtArg): i64;
fn spfArgAsUnsigned(arg: FmtArg): u64;
fn spfArgAsFloat(arg: FmtArg): f64;
fn spfFormatFloat(val: f64, prec: usize, plus_sign: bool, width: usize, left_align: bool, zero_pad: bool, out: []u8, pos: usize): usize;
fn sprintf(fmt: string, args: &[]FmtArg, out: []u8): usize;
```

## Minimal Usage

```cp
import { sprintf } from "sprintf-js";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sprintf("example", /* args: &[]FmtArg */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sprintf-js"`; install via `cap add sprintf-js`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
