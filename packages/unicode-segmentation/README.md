[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `unicode-segmentation`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-segmentation` ([https://github.com/unicode-rs/unicode-segmentation](https://github.com/unicode-rs/unicode-segmentation))

Zero-allocation **UAX #29** Unicode Extended Grapheme Cluster and Word Boundary segmentation engine for CP (clean-room MIT implementation inspired by Rust's `unicode-segmentation`).

## Features

- **Zero Heap Allocation**: Operates directly on UTF-8 `string` slices and returns lightweight `GraphemeSpan { start, end }` byte-offset views.
- **Full UTF-8 Decoding**: Handles 1- to 4-byte UTF-8 sequences with safe `U+FFFD` fallback on malformed byte sequences.
- **UAX #29 Extended Grapheme Clusters**:
  - CRLF (`\r\n`) single-cluster pairing (GB3)
  - Combining diacritical marks, Variation Selectors (`U+FE00..U+FE0F`), and Emoji Fitzpatrick skin-tone modifiers (`U+1F3FB..U+1F3FF`) (GB9)
  - Emoji ZWJ sequences (`👨‍👩‍👧‍👦`, `👩🏽‍💻`, `❤️‍🔥`) via `Extended_Pictographic Extend* ZWJ × Extended_Pictographic` (GB11)
  - Regional Indicator flag pairs (`🇺🇸🇨🇳`) (GB12/GB13)
  - Hangul Jamo & precomposed syllable composition (`L`/`V`/`T`/`LV`/`LVT`) (GB6–GB8)
- **UAX #29 Word Boundary Segmentation**:
  - `WordIter.next()` returns all word-boundary segments (words, whitespace, punctuation)
  - `WordIter.nextUnicodeWord()` yields meaningful Unicode words, supporting ASCII/Latin alphanumerics, apostrophe contractions (`don't`), decimals/thousands (`3.14`, `1,000`), and per-character CJK word splitting.

## Usage

```typescript
import {
    GraphemeSpan,
    GraphemeIter,
    countGraphemes,
    nthGrapheme,
    sliceGraphemes,
    WordIter,
    countUnicodeWords,
} from "unicode-segmentation";

const n: usize = countGraphemes("a\r\ne\u0301👨‍👩‍👧‍👦🇺🇸"); // 5

let it = GraphemeIter.new("Hello 🌍!");
while (true) {
    const sp = it.next();
    if (sp == null) { break; }
    const s = sp.?;
    // slice is src[s.start..s.end]
}

const sub = sliceGraphemes("café🚀!", 1, 3); // "afé" (or "fé🚀" depending on index)
const words: usize = countUnicodeWords("Don't stop at 3.14, 你好!"); // 6
```

## API

- `struct GraphemeSpan { start: usize, end: usize }` with `byteLen(): usize`
- `struct GraphemeIter` with `new(src: string)`, `next(*_): GraphemeSpan?`, `reset(*_)`
- `countGraphemes(src: string): usize`
- `nthGrapheme(src: string, idx: usize): GraphemeSpan?`
- `sliceGraphemes(src: string, start_idx: usize, max_count: usize): GraphemeSpan`
- `struct WordIter` with `new(src: string)`, `next(*_): GraphemeSpan?`, `nextUnicodeWord(*_): GraphemeSpan?`, `reset(*_)`
- `countUnicodeWords(src: string): usize`

## License

MIT
