[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# words-count

Zero-allocation multilingual (Latin + CJK) word/character/line counter and reading-time estimator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:words-count` ([https://github.com/magiclen/words-count](https://github.com/magiclen/words-count)) / `npm:word-count` ([https://github.com/yuehu/word-count](https://github.com/yuehu/word-count))（原库名 `words-count` → CP 包名 `words-count`）

| Metadata | Value |
| --- | --- |
| Package | `words-count` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add words-count
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TextCounts` | `class` | Fields: `bytes: usize, chars: usize, words: usize, cjk_chars: usize, latin_words: usize, lines: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isCjkCodepoint` | `(cp: u32): bool` | Checks predicate `isCjkCodepoint` and returns `bool`. |
| `wcIsLatinWordChar` | `(cp: u32): bool` | Invokes `wcIsLatinWordChar` and returns `bool`. |
| `wcNextCodepoint` | `(text: &[]u8, pos: usize, next_pos: &mut usize): u32` | Invokes `wcNextCodepoint` and returns `u32`. |
| `analyze` | `(text: string): TextCounts` | Invokes `analyze` and returns `TextCounts`. |
| `countWords` | `(text: string): usize` | Invokes `countWords` and returns `usize`. |
| `estimateReadingSeconds` | `(text: string, words_per_minute: u32): u32` | Invokes `estimateReadingSeconds` and returns `u32`. |

## Minimal Example

```cp
import { analyze } from "words-count";

pub fn main() {
    let result = analyze("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
