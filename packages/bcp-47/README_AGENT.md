# bcp-47 — AI Agent Quick Ref

Zero-allocation RFC 5646 / BCP-47 language tag parser, canonicalizer, and fallback locale matcher for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Bcp47Tag,
    SubtagSpan,
    eqSubtagNormalized,
    isAllAlpha,
    isAllDigit,
    isAllAlphaNum,
    isVariantSubtag,
    isSingletonSubtag,
} from "bcp-47";
```

## Core Types & Constants

```cp
class Bcp47Tag { valid: bool, language: string, script: string, region: string, /* +3 fields */ } // methods: invalid(...)
class SubtagSpan { start: usize, end: usize }
```

## API Signatures

```cp
fn eqSubtagNormalized(a: string, b: string): bool;
fn isAllAlpha(s: string): bool;
fn isAllDigit(s: string): bool;
fn isAllAlphaNum(s: string): bool;
fn isVariantSubtag(s: string): bool;
fn isSingletonSubtag(s: string): bool;
fn isPrivateIntro(s: string): bool;
fn parseBcp47(raw: string): Bcp47Tag;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn appendLowerHyphen(out: []u8, start_pos: usize, s: string): usize;
fn appendUpper(out: []u8, start_pos: usize, s: string): usize;
fn appendTitle(out: []u8, start_pos: usize, s: string): usize;
fn canonicalizeBcp47(tag: Bcp47Tag, out: []u8): string;
fn scoreLocaleMatch(req: Bcp47Tag, cand: Bcp47Tag): u32;
fn matchLocale(requested: Bcp47Tag, supported: &[]string): string?;
```

## Minimal Usage

```cp
import { isAllAlpha } from "bcp-47";

pub fn main() {
    let result = isAllAlpha("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bcp-47"`; install via `cap add bcp-47`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
