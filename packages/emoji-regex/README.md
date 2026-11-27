[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# emoji-regex

Zero-allocation Unicode RGI Emoji UTF-8 scanner and extractor (npm emoji-regex parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`emoji-regex`](https://github.com/mathiasbynens/emoji-regex)（`MIT`）（包名 `emoji-regex`）。

| Metadata | Value |
| --- | --- |
| Package | `emoji-regex` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add emoji-regex
```

```cp
import {
    EmojiSpan,
    EmjDecoded,
    emjDecodeUtf8,
    emjIsRegionalIndicator,
    emjIsSkinTone,
    emjIsKeycapBase,
    emjIsTagChar,
    isEmojiCodePoint,
} from "emoji-regex";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EmojiSpan` | `class` | Fields: `start: usize, end: usize` |
| `EmjDecoded` | `class` | Fields: `cp: u32, len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `emjDecodeUtf8` | `(s: string, pos: usize): EmjDecoded` | Invokes `emjDecodeUtf8` and returns `EmjDecoded`. |
| `emjIsRegionalIndicator` | `(cp: u32): bool` | Invokes `emjIsRegionalIndicator` and returns `bool`. |
| `emjIsSkinTone` | `(cp: u32): bool` | Invokes `emjIsSkinTone` and returns `bool`. |
| `emjIsKeycapBase` | `(cp: u32): bool` | Invokes `emjIsKeycapBase` and returns `bool`. |
| `emjIsTagChar` | `(cp: u32): bool` | Invokes `emjIsTagChar` and returns `bool`. |
| `isEmojiCodePoint` | `(cp: u32): bool` | Checks predicate `isEmojiCodePoint` and returns `bool`. |
| `emjConsumeModifiers` | `(s: string, start_pos: usize): usize` | Invokes `emjConsumeModifiers` and returns `usize`. |
| `findNextEmoji` | `(s: string, from_pos: usize): EmojiSpan?` | Queries/retrieves data via `findNextEmoji` returning `EmojiSpan?`. |
| `countEmojis` | `(s: string): usize` | Invokes `countEmojis` and returns `usize`. |
| `containsEmoji` | `(s: string): bool` | Invokes `containsEmoji` and returns `bool`. |
| `stripEmojis` | `(s: string, out: []u8): usize` | Invokes `stripEmojis` and returns `usize`. |

## Minimal Example

```cp
import { emjDecodeUtf8 } from "emoji-regex";

pub fn main() {
    let result = emjDecodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
