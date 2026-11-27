# picomatch — AI Agent Quick Ref

Zero-allocation glob matcher & static base scanner (npm picomatch v4 parity) with globstar, char classes, brace expansion, dotfile protection, and negation for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PicoOptions,
    pmCharEq,
    pmBasenameSlice,
    pmIsLeadingDot,
    pmCountLeadingNegations,
    pmHasHiddenSegment,
    pmFindClassEnd,
    pmMatchCharClass,
} from "picomatch";
```

## Core Types & Constants

```cp
class PicoOptions { dot: bool, nocase: bool, basename: bool, noglobstar: bool } // methods: defaults(...)
```

## API Signatures

```cp
fn pmCharEq(a: u8, b: u8, nocase: bool): bool;
fn pmBasenameSlice(path: string): string;
fn pmIsLeadingDot(path: string, pIdx: usize): bool;
fn pmCountLeadingNegations(pat: string): usize;
fn pmHasHiddenSegment(path: string, pIdx: usize): bool;
fn pmFindClassEnd(pat: string, gIdx: usize): usize;
fn pmMatchCharClass(ch: u8, pat: string, gIdx: usize, endIdx: usize, nocase: bool): bool;
fn pmMatchCore(path: string, pIdx: usize, pat: string, gIdx: usize, opts: PicoOptions, depth: usize): bool;
fn pmFindBraceGroup(pat: string, outBounds: []usize): bool;
fn pmMatchWithBraces(path: string, pat: string, opts: PicoOptions, depth: usize): bool;
fn pmMatchNormalized(normPath: string, pat: string, opts: PicoOptions): bool;
fn pmSegmentHasGlob(seg: string): bool;
fn isMatchWithOptions(path: string, pattern: string, opts: PicoOptions): bool;
fn isMatch(path: string, pattern: string): bool;
fn isMatchAny(path: string, patterns: &[]string, opts: PicoOptions): bool;
fn scanBase(pattern: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isMatchWithOptions } from "picomatch";

pub fn main() {
    let result = isMatchWithOptions("example", "example", /* opts: PicoOptions */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"picomatch"`; install via `cap add picomatch`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
