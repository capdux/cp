[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# reading-time

Zero-allocation bilingual EN/CJK document statistics, reading-time estimator, and LLM token estimator for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:reading-time` ([https://github.com/ngryman/reading-time](https://github.com/ngryman/reading-time))（原库名 `reading-time` → CP 包名 `reading-time`）

| Metadata | Value |
| --- | --- |
| Package | `reading-time` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add reading-time
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TextStats` | `class` | Fields: `bytes: usize, chars: usize, ascii_words: usize, cjk_chars: usize, total_words: usize, lines: usize, +6 more` · Methods: `empty` |
| `DecodedChar` | `class` | Fields: `cp: u32, len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `decodeUtf8` | `(s: string, pos: usize): DecodedChar` | Parses input via `decodeUtf8` and returns `DecodedChar`. |
| `isCjkChar` | `(cp: u32): bool` | Checks predicate `isCjkChar` and returns `bool`. |
| `isWordCoreChar` | `(cp: u32): bool` | Checks predicate `isWordCoreChar` and returns `bool`. |
| `isWsCp` | `(cp: u32): bool` | Checks predicate `isWsCp` and returns `bool`. |
| `isSentenceEnd` | `(cp: u32): bool` | Checks predicate `isSentenceEnd` and returns `bool`. |
| `analyzeTextWithWpm` | `(text: string, en_wpm: usize, cjk_cpm: usize): TextStats` | Invokes `analyzeTextWithWpm` and returns `TextStats`. |
| `analyzeText` | `(text: string): TextStats` | Invokes `analyzeText` and returns `TextStats`. |
| `formatReadingTime` | `(stats: TextStats, zh: bool, out: []u8): string` | Serializes/formats data via `formatReadingTime` returning `string`. |

## Minimal Example

```cp
import { decodeUtf8 } from "reading-time";

pub fn main() {
    let result = decodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
