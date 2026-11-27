# glob-parent — AI Agent Quick Ref

Zero-allocation glob static parent directory extractor, glob detector, and escaper (glob-parent + is-glob parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    GlobSplit,
    findMatchingBracket,
    findMatchingBrace,
    findFirstGlobPos,
    stripLeadingNegation,
    trimTrailingSlashes,
    isGlob,
    globParent,
} from "glob-parent";
```

## Core Types & Constants

```cp
class GlobSplit { base: string, glob: string, is_glob: bool }
```

## API Signatures

```cp
fn findMatchingBracket(s: string, openIdx: usize): usize?;
fn findMatchingBrace(s: string, openIdx: usize): usize?;
fn findFirstGlobPos(s: string): usize?;
fn stripLeadingNegation(raw: string): string;
fn trimTrailingSlashes(s: string): string;
fn isGlob(pattern: string): bool;
fn globParent(pattern: string): string;
fn splitGlobBase(pattern: string): GlobSplit;
fn isGlobSpecial(c: u8): bool;
fn escapeGlob(raw: string, out: []u8): string;
```

## Minimal Usage

```cp
import { isGlob } from "glob-parent";

pub fn main() {
    let result = isGlob("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"glob-parent"`; install via `cap add glob-parent`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
