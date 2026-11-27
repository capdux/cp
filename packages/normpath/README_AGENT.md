# normpath — AI Agent Quick Ref

Zero-heap cross-platform lexical path normalization (POSIX, Windows drive/UNC/verbatim) in pure CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    npIsAsciiAlpha,
    npToUpper,
    npIsWinSep,
    npWriteByte,
    npWriteSlice,
    npSegEqDot,
    npSegEqDotDot,
    npNormalizePosixTwo,
} from "normpath";
```

## API Signatures

```cp
fn npIsAsciiAlpha(b: u8): bool;
fn npToUpper(b: u8): u8;
fn npIsWinSep(b: u8): bool;
fn npWriteByte(out: []u8, pos: usize, b: u8): usize;
fn npWriteSlice(out: []u8, pos: usize, src: string): usize;
fn npSegEqDot(src: string, start: usize, len: usize): bool;
fn npSegEqDotDot(src: string, start: usize, len: usize): bool;
fn npNormalizePosixTwo(p1: string, p2: string, hasP2: bool, out: []u8): usize;
fn normalizePosix(path: string, out: []u8): usize;
fn joinNormalized(base: string, rel: string, out: []u8): usize;
fn isVerbatimWindows(path: string): bool;
fn stripVerbatimPrefix(path: string): string;
fn npHasUncTag(path: string, off: usize): bool;
fn normalizeWindows(path: string, out: []u8): usize;
fn npWinIsAbsolute(path: string): bool;
fn joinNormalizedWindows(base: string, rel: string, out: []u8): usize;
fn hasParentTraversal(path: string): bool;
fn escapesRoot(path: string): bool;
fn isNormalized(path: string): bool;
```

## Minimal Usage

```cp
import { isVerbatimWindows } from "normpath";

pub fn main() {
    let result = isVerbatimWindows("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"normpath"`; install via `cap add normpath`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
