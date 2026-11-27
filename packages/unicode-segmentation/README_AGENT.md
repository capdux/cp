# unicode-segmentation — AI Agent Quick Ref

Zero-allocation UAX #29 Unicode extended grapheme cluster and word boundary segmentation for CP (clean-room MIT). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    GraphemeSpan,
    UsegRune,
    GraphemeIter,
    WordIter,
    usegDecodeUtf8,
    usegIsExtend,
    usegIsPrepend,
    usegIsSpacingMark,
} from "unicode-segmentation";
```

## Core Types & Constants

```cp
class GraphemeSpan { start: usize, end: usize } // methods: byteLen(...)
class UsegRune { cp: u32, len: usize }
class GraphemeIter { src: string, pos: usize } // methods: new(...) +2
class WordIter { src: string, pos: usize } // methods: new(...) +3
```

## API Signatures

```cp
fn usegDecodeUtf8(src: string, offset: usize): UsegRune;
fn usegIsExtend(cp: u32): bool;
fn usegIsPrepend(cp: u32): bool;
fn usegIsSpacingMark(cp: u32): bool;
fn usegIsControl(cp: u32): bool;
fn usegIsExtPic(cp: u32): bool;
fn usegGraphemeProp(cp: u32): u8;
fn usegNextGraphemeEnd(src: string, start: usize): usize;
fn countGraphemes(src: string): usize;
fn nthGrapheme(src: string, idx: usize): GraphemeSpan?;
fn sliceGraphemes(src: string, start_idx: usize, max_count: usize): GraphemeSpan;
fn usegIsCjkWordChar(cp: u32): bool;
fn usegIsALetter(cp: u32): bool;
fn usegIsNumeric(cp: u32): bool;
fn usegWordProp(cp: u32): u8;
fn usegPeekWordProp(src: string, offset: usize): u8;
fn usegIsWordSpan(src: string, start: usize, end: usize): bool;
fn countUnicodeWords(src: string): usize;
```

## Minimal Usage

```cp
import { usegDecodeUtf8 } from "unicode-segmentation";

pub fn main() {
    let result = usegDecodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unicode-segmentation"`; install via `cap add unicode-segmentation`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
