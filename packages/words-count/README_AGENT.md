# words-count — AI Agent Quick Ref

Zero-allocation multilingual (Latin + CJK) word/character/line counter and reading-time estimator for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TextCounts,
    isCjkCodepoint,
    wcIsLatinWordChar,
    wcNextCodepoint,
    analyze,
    countWords,
    estimateReadingSeconds,
} from "words-count";
```

## Core Types & Constants

```cp
class TextCounts { bytes: usize, chars: usize, words: usize, cjk_chars: usize, /* +2 fields */ }
```

## API Signatures

```cp
fn isCjkCodepoint(cp: u32): bool;
fn wcIsLatinWordChar(cp: u32): bool;
fn wcNextCodepoint(text: &[]u8, pos: usize, next_pos: &mut usize): u32;
fn analyze(text: string): TextCounts;
fn countWords(text: string): usize;
fn estimateReadingSeconds(text: string, words_per_minute: u32): u32;
```

## Minimal Usage

```cp
import { analyze } from "words-count";

pub fn main() {
    let result = analyze("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"words-count"`; install via `cap add words-count`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
