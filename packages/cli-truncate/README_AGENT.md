# cli-truncate — AI Agent Quick Ref

Zero-allocation ANSI-aware CLI string truncator (end/start/middle, CJK double-width, space boundary preference) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TruncatePosition,
    TruncateOptions,
    ctWriteBytes,
    ctSkipAnsi,
    ctUtf8Len,
    ctFindLastSpaceLe,
    ctIsSpaceAtCol,
    ctFindFirstSpaceGe,
} from "cli-truncate";
```

## Core Types & Constants

```cp
enum TruncatePosition { END, START, MIDDLE }
class TruncateOptions { position: TruncatePosition, prefer_truncation_on_space: bool, ellipsis: string } // methods: defaults(...)
```

## API Signatures

```cp
fn ctWriteBytes(out: []u8, pos: usize, src: string): usize;
fn ctSkipAnsi(text: string, pos: usize): usize;
fn ctUtf8Len(text: string, pos: usize): usize;
fn ctFindLastSpaceLe(text: string, max_col: usize): usize;
fn ctIsSpaceAtCol(text: string, target_col: usize): bool;
fn ctFindFirstSpaceGe(text: string, min_col: usize, limit_col: usize): usize;
fn cliTruncateInto(text: string, columns: usize, opts: TruncateOptions, out: []u8): usize;
fn cliTruncateEndInto(text: string, columns: usize, out: []u8): usize;
fn cliTruncateStartInto(text: string, columns: usize, out: []u8): usize;
fn cliTruncateMiddleInto(text: string, columns: usize, out: []u8): usize;
```

## Minimal Usage

```cp
import { ctWriteBytes } from "cli-truncate";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ctWriteBytes(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cli-truncate"`; install via `cap add cli-truncate`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
