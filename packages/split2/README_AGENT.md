# split2 — AI Agent Quick Ref

Zero-allocation streaming line splitter across arbitrary chunk boundaries (LF & CRLF) in pure CP (`v1.0.0`, `ISC`)

## Import

```cp
import { LineSplitter, sp2Min, sp2CopyBytes, splitLines } from "split2";
```

## Core Types & Constants

```cp
class LineSplitter { buf: [4096]u8, len: usize, max_line_len: usize, skip_empty: bool, /* +1 fields */ } // methods: new(...) +6
```

## API Signatures

```cp
fn sp2Min(a: usize, b: usize): usize;
fn sp2CopyBytes(dst: []u8, dst_off: usize, src: &[]u8, n: usize);
fn splitLines(input: string, out_slices: []string): usize;
```

## Minimal Usage

```cp
import { splitLines } from "split2";

pub fn main() {
    let result = splitLines("example", /* out_slices: []string */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"split2"`; install via `cap add split2`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
