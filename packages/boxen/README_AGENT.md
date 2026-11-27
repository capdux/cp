# boxen — AI Agent Quick Ref

Zero-allocation CLI terminal box renderer with 6 Unicode/ASCII border styles, embedded titles, alignment, CJK double-width & ANSI escape awareness (boxen parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BorderStyle,
    TextAlign,
    BoxOptions,
    BorderChars,
    Utf8Step,
    borderForStyle,
    decodeUtf8At,
    codepointWidth,
} from "boxen";
```

## Core Types & Constants

```cp
enum BorderStyle { Single, Double, Round, Bold, Classic, SingleDouble }
enum TextAlign { Left, Center, Right }
class BoxOptions { style: BorderStyle, text_align: TextAlign, padding_x: u8, padding_y: u8, /* +3 fields */ } // methods: defaults(...)
class BorderChars { tl: string, top: string, tr: string, left: string, /* +4 fields */ }
class Utf8Step { cp: u32, width: usize }
```

## API Signatures

```cp
fn borderForStyle(style: BorderStyle): BorderChars;
fn decodeUtf8At(s: string, i: usize): Utf8Step;
fn codepointWidth(cp: u32): usize;
fn displayWidth(s: string): usize;
fn pushByte(out: []u8, pos: usize, b: u8): usize;
fn pushStr(out: []u8, pos: usize, s: string): usize;
fn pushRepeat(out: []u8, pos: usize, s: string, count: usize): usize;
fn pushSpaces(out: []u8, pos: usize, count: usize): usize;
fn renderBox(text: string, opts: BoxOptions, out: []u8): string;
fn box(text: string, out: []u8): string;
fn titledBox(title: string, text: string, style: BorderStyle, out: []u8): string;
```

## Minimal Usage

```cp
import { decodeUtf8At } from "boxen";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"boxen"`; install via `cap add boxen`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
