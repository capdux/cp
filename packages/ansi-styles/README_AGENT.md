# ansi-styles — AI Agent Quick Ref

ANSI escape codes for styling strings in the terminal (clean-room CP implementation of npm ansi-styles) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StylePair,
    asPushStr,
    asPushU8,
    wrapStyle,
    formatAnsi256,
    formatAnsi16m,
    formatRgbToAnsi16,
    formatRgbToAnsi256,
} from "ansi-styles";
```

## Core Types & Constants

```cp
class StylePair { open: string, close: string }
```

## API Signatures

```cp
fn asPushStr(out: []u8, pos: usize, s: string): usize;
fn asPushU8(out: []u8, pos: usize, v: u8): usize;
fn wrapStyle(style: StylePair, text: string, out: []u8): usize;
fn formatAnsi256(code: u8, is_bg: bool, out: []u8): usize;
fn formatAnsi16m(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize;
fn formatRgbToAnsi16(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize;
fn formatRgbToAnsi256(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize;
fn formatHexToAnsi16m(hex: string, is_bg: bool, out: []u8): usize;
fn formatKeywordToAnsi16m(name: string, is_bg: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatHexToAnsi16m } from "ansi-styles";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatHexToAnsi16m("example", false, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ansi-styles"`; install via `cap add ansi-styles`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
