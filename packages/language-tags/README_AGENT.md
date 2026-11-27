# language-tags — AI Agent Quick Ref

Zero-heap BCP 47 / RFC 5646 language tag parser, canonical formatter, and RFC 4647 range matcher in pure CP (`v1.0.9`, `MIT`)

## Import

```cp
import {
    Bcp47Tag,
    ltgIsAlpha,
    ltgIsDigit,
    ltgIsAlnum,
    ltgIsSep,
    ltgToLower,
    ltgToUpper,
    ltgAllAlpha,
} from "language-tags";
```

## Core Types & Constants

```cp
class Bcp47Tag { language: string, script: string, region: string, variant: string, /* +1 fields */ }
```

## API Signatures

```cp
fn ltgIsAlpha(c: u8): bool;
fn ltgIsDigit(c: u8): bool;
fn ltgIsAlnum(c: u8): bool;
fn ltgIsSep(c: u8): bool;
fn ltgToLower(c: u8): u8;
fn ltgToUpper(c: u8): u8;
fn ltgAllAlpha(s: string): bool;
fn ltgAllDigit(s: string): bool;
fn ltgAllAlnum(s: string): bool;
fn ltgInvalidTag(): Bcp47Tag;
fn parseLanguageTag(tag: string): Bcp47Tag;
fn ltgPushByte(out: []u8, pos: usize, b: u8): usize;
fn formatCanonicalTag(t: Bcp47Tag, out: []u8): usize;
fn matchesLanguageRange(tag: string, range: string): bool;
```

## Minimal Usage

```cp
import { parseLanguageTag } from "language-tags";

pub fn main() {
    let result = parseLanguageTag("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"language-tags"`; install via `cap add language-tags`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
