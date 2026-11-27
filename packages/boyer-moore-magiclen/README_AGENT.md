# boyer-moore-magiclen — AI Agent Quick Ref

Zero-heap Boyer-Moore-Horspool sublinear substring search with reusable 256-byte bad-character table, reverse search, counting, and ASCII case-insensitive search. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BmhPattern,
    compilePattern,
    findWithCompiled,
    findFirst,
    findLast,
    countNonOverlapping,
    findIgnoreCaseAscii,
} from "boyer-moore-magiclen";
```

## Core Types & Constants

```cp
class BmhPattern { shift: [256]usize, pat_len: usize }
```

## API Signatures

```cp
fn compilePattern(needle: &[]u8): BmhPattern;
fn findWithCompiled(haystack: &[]u8, needle: &[]u8, compiled: BmhPattern): i64;
fn findFirst(haystack: &[]u8, needle: &[]u8): i64;
fn findLast(haystack: &[]u8, needle: &[]u8): i64;
fn countNonOverlapping(haystack: &[]u8, needle: &[]u8): usize;
fn findIgnoreCaseAscii(haystack: string, needle: string): i64;
```

## Minimal Usage

```cp
import { findIgnoreCaseAscii } from "boyer-moore-magiclen";

pub fn main() {
    let result = findIgnoreCaseAscii("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"boyer-moore-magiclen"`; install via `cap add boyer-moore-magiclen`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
