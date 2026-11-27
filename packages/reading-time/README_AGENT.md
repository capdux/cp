# reading-time — AI Agent Quick Ref

Zero-allocation bilingual EN/CJK document statistics, reading-time estimator, and LLM token estimator for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TextStats,
    DecodedChar,
    decodeUtf8,
    isCjkChar,
    isWordCoreChar,
    isWsCp,
    isSentenceEnd,
    analyzeTextWithWpm,
} from "reading-time";
```

## Core Types & Constants

```cp
class TextStats { bytes: usize, chars: usize, ascii_words: usize, cjk_chars: usize, /* +8 fields */ } // methods: empty(...)
class DecodedChar { cp: u32, len: usize }
```

## API Signatures

```cp
fn decodeUtf8(s: string, pos: usize): DecodedChar;
fn isCjkChar(cp: u32): bool;
fn isWordCoreChar(cp: u32): bool;
fn isWsCp(cp: u32): bool;
fn isSentenceEnd(cp: u32): bool;
fn analyzeTextWithWpm(text: string, en_wpm: usize, cjk_cpm: usize): TextStats;
fn analyzeText(text: string): TextStats;
fn formatReadingTime(stats: TextStats, zh: bool, out: []u8): string;
```

## Minimal Usage

```cp
import { decodeUtf8 } from "reading-time";

pub fn main() {
    let result = decodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"reading-time"`; install via `cap add reading-time`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
