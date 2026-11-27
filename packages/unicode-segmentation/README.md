[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unicode-segmentation

Zero-allocation UAX #29 Unicode extended grapheme cluster and word boundary segmentation for CP (clean-room MIT).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-segmentation` ([https://github.com/unicode-rs/unicode-segmentation](https://github.com/unicode-rs/unicode-segmentation))

| Metadata | Value |
| --- | --- |
| Package | `unicode-segmentation` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unicode-segmentation
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GraphemeSpan` | `class` | Fields: `start: usize, end: usize` · Methods: `byteLen` |
| `UsegRune` | `class` | Fields: `cp: u32, len: usize` |
| `GraphemeIter` | `class` | Fields: `src: string, pos: usize` · Methods: `new, next, reset` |
| `WordIter` | `class` | Fields: `src: string, pos: usize` · Methods: `new, reset, next, nextUnicodeWord` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `usegDecodeUtf8` | `(src: string, offset: usize): UsegRune` | Invokes `usegDecodeUtf8` and returns `UsegRune`. |
| `usegIsExtend` | `(cp: u32): bool` | Invokes `usegIsExtend` and returns `bool`. |
| `usegIsPrepend` | `(cp: u32): bool` | Invokes `usegIsPrepend` and returns `bool`. |
| `usegIsSpacingMark` | `(cp: u32): bool` | Invokes `usegIsSpacingMark` and returns `bool`. |
| `usegIsControl` | `(cp: u32): bool` | Invokes `usegIsControl` and returns `bool`. |
| `usegIsExtPic` | `(cp: u32): bool` | Invokes `usegIsExtPic` and returns `bool`. |
| `usegGraphemeProp` | `(cp: u32): u8` | Invokes `usegGraphemeProp` and returns `u8`. |
| `usegNextGraphemeEnd` | `(src: string, start: usize): usize` | Invokes `usegNextGraphemeEnd` and returns `usize`. |
| `countGraphemes` | `(src: string): usize` | Invokes `countGraphemes` and returns `usize`. |
| `nthGrapheme` | `(src: string, idx: usize): GraphemeSpan?` | Invokes `nthGrapheme` and returns `GraphemeSpan?`. |
| `sliceGraphemes` | `(src: string, start_idx: usize, max_count: usize): GraphemeSpan` | Invokes `sliceGraphemes` and returns `GraphemeSpan`. |
| `usegIsCjkWordChar` | `(cp: u32): bool` | Invokes `usegIsCjkWordChar` and returns `bool`. |
| `usegIsALetter` | `(cp: u32): bool` | Invokes `usegIsALetter` and returns `bool`. |
| `usegIsNumeric` | `(cp: u32): bool` | Invokes `usegIsNumeric` and returns `bool`. |
| `usegWordProp` | `(cp: u32): u8` | Invokes `usegWordProp` and returns `u8`. |
| `usegPeekWordProp` | `(src: string, offset: usize): u8` | Invokes `usegPeekWordProp` and returns `u8`. |
| `usegIsWordSpan` | `(src: string, start: usize, end: usize): bool` | Invokes `usegIsWordSpan` and returns `bool`. |
| `countUnicodeWords` | `(src: string): usize` | Invokes `countUnicodeWords` and returns `usize`. |

## Minimal Example

```cp
import { usegDecodeUtf8 } from "unicode-segmentation";

pub fn main() {
    let result = usegDecodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
