[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# title-case

Zero-allocation AP/Chicago style English title-case, sentence-case, and capitalization engine in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:title-case` ([https://github.com/blakeembrey/change-case](https://github.com/blakeembrey/change-case)) / `crates.io:titlecase` ([https://github.com/wezm/titlecase](https://github.com/wezm/titlecase))

| Metadata | Value |
| --- | --- |
| Package | `title-case` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add title-case
```

```cp
import {
    isUpper,
    isLower,
    isSmallWord,
    isWordChar,
    isAllCapsAcronym,
    hasMoreWords,
    toTitleCase,
    toSentenceCase,
} from "title-case";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isUpper` | `(b: u8): bool` | Checks predicate `isUpper` and returns `bool`. |
| `isLower` | `(b: u8): bool` | Checks predicate `isLower` and returns `bool`. |
| `isSmallWord` | `(word: string): bool` | Checks predicate `isSmallWord` and returns `bool`. |
| `isWordChar` | `(b: u8): bool` | Checks predicate `isWordChar` and returns `bool`. |
| `isAllCapsAcronym` | `(word: string): bool` | Checks predicate `isAllCapsAcronym` and returns `bool`. |
| `hasMoreWords` | `(input: string, start_pos: usize): bool` | Checks predicate `hasMoreWords` and returns `bool`. |
| `toTitleCase` | `(input: string, out: []u8): usize` | Invokes `toTitleCase` and returns `usize`. |
| `toSentenceCase` | `(input: string, out: []u8): usize` | Invokes `toSentenceCase` and returns `usize`. |
| `capitalizeFirst` | `(input: string, out: []u8): usize` | Invokes `capitalizeFirst` and returns `usize`. |

## Minimal Example

```cp
import { isSmallWord } from "title-case";

pub fn main() {
    let result = isSmallWord("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
