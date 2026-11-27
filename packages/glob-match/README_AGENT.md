# glob-match — AI Agent Quick Ref

Zero-allocation, anti-backtracking path glob and brace alternation matching engine for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    GlobSpan,
    gbmFindClassClose,
    gbmMatchCharClass,
    gbmMatchCore,
    gbmFindBracePair,
    gbmCopySlice,
    gbmMatchWithBraces,
    globMatch,
} from "glob-match";
```

## Core Types & Constants

```cp
class GlobSpan { start: usize, end: usize }
```

## API Signatures

```cp
fn gbmFindClassClose(pattern: string, open_idx: usize): usize;
fn gbmMatchCharClass(pattern: string, open_idx: usize, close_idx: usize, ch: u8): bool;
fn gbmMatchCore(pattern: string, path: string, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool;
fn gbmFindBracePair(pattern: string, open_out: &mut usize, close_out: &mut usize): bool;
fn gbmCopySlice(dst: []u8, pos: usize, src: string, s: usize, e: usize): usize;
fn gbmMatchWithBraces(pattern: string, path: string, depth: usize, steps: &mut usize, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool;
fn globMatch(pattern: string, path: string): bool;
fn globMatchAny(patterns: &[]string, path: string): bool;
fn globMatchCaptures(pattern: string, path: string, out_caps: []GlobSpan): usize;
fn isStaticGlob(pattern: string): bool;
```

## Minimal Usage

```cp
import { isStaticGlob } from "glob-match";

pub fn main() {
    let result = isStaticGlob("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"glob-match"`; install via `cap add glob-match`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
