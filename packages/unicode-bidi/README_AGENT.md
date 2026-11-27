# unicode-bidi — AI Agent Quick Ref

Zero-allocation Unicode Bidirectional Algorithm (UAX #9) classifier, base direction resolver, visual run splitter, and bracket mirroring for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BidiRun,
    Utf8Step,
    decodeUtf8At,
    bidiClass,
    baseDirection,
    hasRtl,
    isLtrOnly,
    resolveCharLevel,
} from "unicode-bidi";
```

## Core Types & Constants

```cp
class BidiRun { start: usize, len: usize, level: u8 }
class Utf8Step { cp: u32, len: usize }
```

## API Signatures

```cp
fn decodeUtf8At(text: string, pos: usize): Utf8Step;
fn bidiClass(cp: u32): u8;
fn baseDirection(text: string): u8;
fn hasRtl(text: string): bool;
fn isLtrOnly(text: string): bool;
fn resolveCharLevel(cls: u8, base_level: u8, prev_level: u8): u8;
fn computeVisualRuns(text: string, out_runs: []BidiRun): usize;
fn mirrorAsciiBracket(ch: u8): u8;
```

## Minimal Usage

```cp
import { decodeUtf8At } from "unicode-bidi";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unicode-bidi"`; install via `cap add unicode-bidi`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
