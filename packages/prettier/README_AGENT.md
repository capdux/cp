# prettier — AI Agent Quick Ref

Zero-heap JSON and CSS pretty-printer formatter (clean-room CP implementation inspired by npm prettier) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FormatOptions,
    pfIsSpace,
    pfPushByte,
    pfPushIndent,
    pfPeekNextNonSpace,
    formatJson,
    formatCss,
} from "prettier";
```

## Core Types & Constants

```cp
class FormatOptions { tab_width: usize, use_tabs: bool, trailing_newline: bool } // methods: defaults(...)
```

## API Signatures

```cp
fn pfIsSpace(c: u8): bool;
fn pfPushByte(out: []u8, pos: usize, b: u8): usize;
fn pfPushIndent(out: []u8, pos: usize, depth: usize, opts: FormatOptions): usize;
fn pfPeekNextNonSpace(s: string, start: usize): u8;
fn formatJson(input: string, opts: FormatOptions, out: []u8): usize;
fn formatCss(input: string, opts: FormatOptions, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatJson } from "prettier";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatJson("example", /* opts: FormatOptions */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"prettier"`; install via `cap add prettier`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
