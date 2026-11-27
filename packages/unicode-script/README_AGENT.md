# unicode-script — AI Agent Quick Ref

Zero-heap UAX #24 Unicode Script property classifier and dominant script detector in pure CP (modeled after Rust unicode-script) (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    Script,
    UscDecoded,
    uscScriptIndex,
    uscIndexToScript,
    scriptOf,
    scriptName,
    isLatin,
    isHan,
} from "unicode-script";
```

## Core Types & Constants

```cp
enum Script { Common, Latin, Greek, Cyrillic, Han, Hiragana, ... }
class UscDecoded { cp: u32, len: usize }
```

## API Signatures

```cp
fn uscScriptIndex(s: Script): usize;
fn uscIndexToScript(idx: usize): Script;
fn scriptOf(cp: u32): Script;
fn scriptName(s: Script): string;
fn isLatin(cp: u32): bool;
fn isHan(cp: u32): bool;
fn isCjkScript(s: Script): bool;
fn uscDecodeUtf8(text: string, pos: usize): UscDecoded;
fn dominantScript(text: string): Script;
```

## Minimal Usage

```cp
import { uscDecodeUtf8 } from "unicode-script";

pub fn main() {
    let result = uscDecodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unicode-script"`; install via `cap add unicode-script`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
