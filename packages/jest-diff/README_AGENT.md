# jest-diff — AI Agent Quick Ref

Zero-heap pure CP Jest-style unified diff engine with ANSI coloring (clean-room MIT implementation of npm jest-diff) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    JdfLine,
    DiffOptions,
    DiffSummary,
    jdfSplitLines,
    jdfWriteBytes,
    jdfWriteStr,
    jdfWriteByte,
    jdfWriteUsize,
} from "jest-diff";
```

## Core Types & Constants

```cp
class JdfLine { start: usize, end: usize }
class DiffOptions { a_annotation: string, b_annotation: string, context_lines: usize, expand: bool, /* +2 fields */ } // methods: defaults(...) +1
class DiffSummary { insertions: usize, deletions: usize, equal: usize, has_diff: bool }
```

## API Signatures

```cp
fn jdfSplitLines(text: string, out_lines: []JdfLine): usize;
fn jdfWriteBytes(out: []u8, pos: usize, src: string, start: usize, end: usize): usize;
fn jdfWriteStr(out: []u8, pos: usize, s: string): usize;
fn jdfWriteByte(out: []u8, pos: usize, b: u8): usize;
fn jdfWriteUsize(out: []u8, pos: usize, val: usize): usize;
fn jdfEmitStyledLine(fmt: PicoFormatter, color_kind: u8, line_text: string, out: []u8, pos: usize): usize;
fn diffStats(expected: string, received: string): DiffSummary;
fn diffLinesUnified(expected: string, received: string, opts: DiffOptions, out: []u8): usize;
```

## Minimal Usage

```cp
import { jdfSplitLines } from "jest-diff";

pub fn main() {
    let result = jdfSplitLines("example", /* out_lines: []JdfLine */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"jest-diff"`; install via `cap add jest-diff`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
