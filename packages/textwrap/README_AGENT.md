# textwrap — AI Agent Quick Ref

Zero-allocation text wrapping, ANSI-aware display width, CJK wide-char breaking, indent/dedent, and truncation engine for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    WrapOptions,
    DecodedChar,
    WrapCursor,
    appendByte,
    decodeUtf8,
    ansiSeqLen,
    isAnsiReset,
    codepointWidth,
} from "textwrap";
```

## Core Types & Constants

```cp
class WrapOptions { width: usize, break_words: bool, preserve_newlines: bool } // methods: defaults(...) +1
class DecodedChar { cp: u32, len: usize }
class WrapCursor { pos: usize, col: usize }
```

## API Signatures

```cp
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn decodeUtf8(s: string, pos: usize): DecodedChar;
fn ansiSeqLen(s: string, pos: usize): usize;
fn isAnsiReset(s: string, pos: usize, seq_len: usize): bool;
fn codepointWidth(cp: u32): usize;
fn isSpaceByte(c: u8): bool;
fn displayWidth(s: string): usize;
fn stripAnsi(s: string, out: []u8): string;
fn writeHardBrokenToken(text: string, tok_start: usize, tok_end: usize, width: usize, out: []u8, start_pos: usize, start_col: usize): WrapCursor;
fn wrapSegment(text: string, seg_start: usize, seg_end: usize, opts: WrapOptions, out: []u8, start_pos: usize): usize;
fn wrapTextWith(text: string, opts: WrapOptions, out: []u8): string;
fn wrapText(text: string, width: usize, out: []u8): string;
fn lineHasContent(text: string, start: usize, end: usize): bool;
fn indent(text: string, prefix: string, out: []u8): string;
fn dedent(text: string, out: []u8): string;
fn truncate(text: string, max_width: usize, ellipsis: string, out: []u8): string;
```

## Minimal Usage

```cp
import { decodeUtf8 } from "textwrap";

pub fn main() {
    let result = decodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"textwrap"`; install via `cap add textwrap`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
