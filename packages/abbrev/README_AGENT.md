# abbrev — AI Agent Quick Ref

Zero-heap unique unambiguous prefix abbreviation generator and resolver for CP (clean-room ISC implementation of npm abbrev). (`v1.0.0`, `ISC`)

## Import

```cp
import {
    AbbrevEntry,
    abvStrEq,
    abvStrLt,
    abvHasPrefix,
    abvLcp,
    abbrev,
    shortestAbbrev,
    resolveAbbrev,
} from "abbrev";
```

## Core Types & Constants

```cp
class AbbrevEntry { abbrev: string, full: string }
```

## API Signatures

```cp
fn abvStrEq(a: string, b: string): bool;
fn abvStrLt(a: string, b: string): bool;
fn abvHasPrefix(word: string, prefix: string): bool;
fn abvLcp(a: string, b: string): usize;
fn abbrev(words: &[]string, out: []AbbrevEntry): usize;
fn shortestAbbrev(words: &[]string, target: string): string?;
fn resolveAbbrev(words: &[]string, prefix: string): string?;
```

## Minimal Usage

```cp
import { abvStrEq } from "abbrev";

pub fn main() {
    let result = abvStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"abbrev"`; install via `cap add abbrev`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
