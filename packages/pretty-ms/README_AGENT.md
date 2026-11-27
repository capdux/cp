# pretty-ms — AI Agent Quick Ref

Zero-allocation millisecond decomposer and human-readable duration formatter composing parse-ms for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PrettyMsOptions,
    prettyAbsI64,
    pushByte,
    pushStr,
    pushU64,
    pushTwoDigits,
    emitUnit,
    prettyMs,
} from "pretty-ms";
```

## Core Types & Constants

```cp
class PrettyMsOptions { verbose: bool, compact: bool, colon_notation: bool, separate_ms: bool, /* +1 fields */ } // methods: defaults(...)
```

## API Signatures

```cp
fn prettyAbsI64(v: i64): u64;
fn pushByte(out: []u8, pos: usize, b: u8): usize;
fn pushStr(out: []u8, pos: usize, s: string): usize;
fn pushU64(out: []u8, pos: usize, val: u64): usize;
fn pushTwoDigits(out: []u8, pos: usize, val: u8): usize;
fn emitUnit(out: []u8, pos: usize, emitted: u8, val: u64, short_unit: string, singular: string, plural: string, verbose: bool): usize;
fn prettyMs(ms: i64, out: []u8): string;
fn prettyMsWith(ms: i64, opts: PrettyMsOptions, out: []u8): string;
```

## Minimal Usage

```cp
import { pushStr } from "pretty-ms";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pushStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pretty-ms"`; install via `cap add pretty-ms`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
