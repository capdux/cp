# slice-ansi — AI Agent Quick Ref

Zero-allocation ANSI-preserving terminal column slicer and truncator with CJK double-width support in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SaUtf8Char,
    SaStyleState,
    saDecodeUtf8,
    saWriteByte,
    saWriteBytes,
    saWriteU16,
    saWriteSgrParam,
    saEmitActiveStyle,
} from "slice-ansi";
```

## Core Types & Constants

```cp
class SaUtf8Char { cp: u32, byte_len: usize }
class SaStyleState { bold: bool, dim: bool, italic: bool, underline: bool, /* +14 fields */ } // methods: empty(...) +4
```

## API Signatures

```cp
fn saDecodeUtf8(s: string, pos: usize): SaUtf8Char;
fn saWriteByte(out: []u8, pos: usize, b: u8): usize;
fn saWriteBytes(out: []u8, pos: usize, src: string): usize;
fn saWriteU16(out: []u8, pos: usize, val: u16): usize;
fn saWriteSgrParam(out: []u8, pos: usize, first: &mut bool, code: u16): usize;
fn saEmitActiveStyle(out: []u8, pos: usize, st: SaStyleState): usize;
fn saEmitCloseStyle(out: []u8, pos: usize, st: SaStyleState): usize;
fn ansiVisibleWidth(input: string): usize;
fn sliceAnsi(input: string, begin: usize, end: usize, out: []u8): usize;
fn sliceAnsiFrom(input: string, begin: usize, out: []u8): usize;
fn truncateAnsi(input: string, max_cols: usize, ellipsis: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { saDecodeUtf8 } from "slice-ansi";

pub fn main() {
    let result = saDecodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"slice-ansi"`; install via `cap add slice-ansi`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
