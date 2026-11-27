# cssesc — AI Agent Quick Ref

Zero-allocation CSSOM-compliant CSS identifier and string escaper in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CssQuoteStyle,
    CssEscOptions,
    CescWriter,
    CescUtf8Res,
    defaultCssEscOptions,
    cescDecodeUtf8,
    cescIsAsciiAlnumOrDash,
    cssescBuf,
} from "cssesc";
```

## Core Types & Constants

```cp
enum CssQuoteStyle { Single, Double }
class CssEscOptions { is_identifier: bool, quotes: CssQuoteStyle, wrap: bool, escape_everything: bool }
class CescWriter { buf: []u8, pos: usize, overflow: bool } // methods: writeByte(...) +1
class CescUtf8Res { cp: u32, next: usize }
```

## API Signatures

```cp
fn defaultCssEscOptions(): CssEscOptions;
fn cescDecodeUtf8(s: string, start: usize): CescUtf8Res;
fn cescIsAsciiAlnumOrDash(cp: u32): bool;
fn cssescBuf(input: string, opts: CssEscOptions, out: []u8): usize;
fn cssesc(input: string, opts: CssEscOptions): string;
```

## Minimal Usage

```cp
import { cescDecodeUtf8 } from "cssesc";

pub fn main() {
    let result = cescDecodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cssesc"`; install via `cap add cssesc`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
