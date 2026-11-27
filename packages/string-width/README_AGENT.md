# string-width — AI Agent Quick Ref

Zero-allocation visual terminal column width calculator (npm string-width v7 parity) composing ansi-strip and unicode-width for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StringWidthOptions,
    SwDecoded,
    swDecodeUtf8,
    swIsRegionalIndicator,
    swIsSkinToneModifier,
    swIsTagChar,
    swBaseCharWidth,
    swMeasurePlain,
} from "string-width";
```

## Core Types & Constants

```cp
class StringWidthOptions { ambiguous_is_narrow: bool, count_ansi_escape_codes: bool } // methods: defaults(...)
class SwDecoded { cp: u32, next_pos: usize }
```

## API Signatures

```cp
fn swDecodeUtf8(s: string, pos: usize): SwDecoded;
fn swIsRegionalIndicator(cp: u32): bool;
fn swIsSkinToneModifier(cp: u32): bool;
fn swIsTagChar(cp: u32): bool;
fn swBaseCharWidth(cp: u32, ambiguous_is_narrow: bool): usize;
fn swMeasurePlain(s: string, ambiguous_is_narrow: bool): usize;
fn swMeasureStrippedChunk(chunk: string, ambiguous_is_narrow: bool): usize;
fn stringWidthWithOptions(input: string, opts: StringWidthOptions): usize;
fn stringWidth(input: string): usize;
fn maxLineWidthWithOptions(multiline: string, opts: StringWidthOptions): usize;
fn maxLineWidth(multiline: string): usize;
fn swCopyBytes(dst: []u8, pos: usize, src: string): usize;
fn swFillBytes(dst: []u8, pos: usize, ch: u8, count: usize): usize;
fn padEndVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize;
fn padStartVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize;
```

## Minimal Usage

```cp
import { swDecodeUtf8 } from "string-width";

pub fn main() {
    let result = swDecodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"string-width"`; install via `cap add string-width`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
