# wrap-ansi — AI Agent Quick Ref

Zero-allocation ANSI-preserving word/hard wrapper and visible-column slicer with CJK double-width awareness (wrap-ansi / slice-ansi parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Utf8Step,
    AnsiStyleState,
    decodeUtf8At,
    codepointWidth,
    scanAnsiCsi,
    isResetSgr,
    startsWithZeroReset,
    pushByte,
} from "wrap-ansi";
```

## Core Types & Constants

```cp
class Utf8Step { cp: u32, width: usize }
class AnsiStyleState { seqs: [MAX_ACTIVE_SGR]string, count: usize } // methods: empty(...) +3
```

## API Signatures

```cp
fn decodeUtf8At(s: string, i: usize): Utf8Step;
fn codepointWidth(cp: u32): usize;
fn scanAnsiCsi(s: string, i: usize): usize;
fn isResetSgr(seq: string): bool;
fn startsWithZeroReset(seq: string): bool;
fn pushByte(out: []u8, pos: usize, b: u8): usize;
fn pushStr(out: []u8, pos: usize, s: string): usize;
fn visibleWidth(s: string): usize;
fn emitLineBreak(out: []u8, pos: usize, style: AnsiStyleState): usize;
fn wrapAnsi(text: string, max_cols: usize, hard_wrap: bool, out: []u8): string;
fn sliceAnsi(text: string, start_col: usize, end_col: usize, out: []u8): string;
```

## Minimal Usage

```cp
import { decodeUtf8At } from "wrap-ansi";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"wrap-ansi"`; install via `cap add wrap-ansi`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
