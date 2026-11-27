# pretty-format — AI Agent Quick Ref

Zero-heap pure CP structured value pretty-printer with indentation, compact mode, and max-depth protection (clean-room MIT implementation of npm pretty-format) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PrettyOptions,
    PrettyWriter,
    pfmtWriteByte,
    pfmtWriteSpaces,
    pfmtWriteF64,
    pfmtWriteStringLiteral,
    formatStringLiteral,
    formatPrimitiveInt,
} from "pretty-format";
```

## Core Types & Constants

```cp
class PrettyOptions { indent: usize, max_depth: usize, min: bool, escape_string: bool, /* +1 fields */ } // methods: defaults(...) +1
class PrettyWriter { opts: PrettyOptions, buf: [2048]u8, len: usize, depth: usize, /* +2 fields */ } // methods: new(...) +17
```

## API Signatures

```cp
fn pfmtWriteByte(out: []u8, pos: usize, b: u8): usize;
fn pfmtWriteSpaces(out: []u8, pos: usize, count: usize): usize;
fn pfmtWriteF64(out: []u8, pos: usize, val: f64): usize;
fn pfmtWriteStringLiteral(out: []u8, pos: usize, s: string, escape_string: bool): usize;
fn formatStringLiteral(s: string, escape: bool, out: []u8): usize;
fn formatPrimitiveInt(v: i64, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatStringLiteral } from "pretty-format";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatStringLiteral("example", false, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pretty-format"`; install via `cap add pretty-format`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
