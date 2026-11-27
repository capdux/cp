[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bcp-47

Zero-allocation RFC 5646 / BCP-47 language tag parser, canonicalizer, and fallback locale matcher for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bcp-47` ([https://github.com/words/bcp-47](https://github.com/words/bcp-47))（原库名 `bcp-47` → CP 包名 `bcp-47`）

| Metadata | Value |
| --- | --- |
| Package | `bcp-47` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bcp-47
```

```cp
import {
    Bcp47Tag,
    SubtagSpan,
    eqSubtagNormalized,
    isAllAlpha,
    isAllDigit,
    isAllAlphaNum,
    isVariantSubtag,
    isSingletonSubtag,
} from "bcp-47";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Bcp47Tag` | `class` | Fields: `valid: bool, language: string, script: string, region: string, variant: string, extension: string, +1 more` · Methods: `invalid` |
| `SubtagSpan` | `class` | Fields: `start: usize, end: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `eqSubtagNormalized` | `(a: string, b: string): bool` | Invokes `eqSubtagNormalized` and returns `bool`. |
| `isAllAlpha` | `(s: string): bool` | Checks predicate `isAllAlpha` and returns `bool`. |
| `isAllDigit` | `(s: string): bool` | Checks predicate `isAllDigit` and returns `bool`. |
| `isAllAlphaNum` | `(s: string): bool` | Checks predicate `isAllAlphaNum` and returns `bool`. |
| `isVariantSubtag` | `(s: string): bool` | Checks predicate `isVariantSubtag` and returns `bool`. |
| `isSingletonSubtag` | `(s: string): bool` | Checks predicate `isSingletonSubtag` and returns `bool`. |
| `isPrivateIntro` | `(s: string): bool` | Checks predicate `isPrivateIntro` and returns `bool`. |
| `parseBcp47` | `(raw: string): Bcp47Tag` | Parses input via `parseBcp47` and returns `Bcp47Tag`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `appendLowerHyphen` | `(out: []u8, start_pos: usize, s: string): usize` | Invokes `appendLowerHyphen` and returns `usize`. |
| `appendUpper` | `(out: []u8, start_pos: usize, s: string): usize` | Invokes `appendUpper` and returns `usize`. |
| `appendTitle` | `(out: []u8, start_pos: usize, s: string): usize` | Invokes `appendTitle` and returns `usize`. |
| `canonicalizeBcp47` | `(tag: Bcp47Tag, out: []u8): string` | Checks predicate `canonicalizeBcp47` and returns `bool`. |
| `scoreLocaleMatch` | `(req: Bcp47Tag, cand: Bcp47Tag): u32` | Invokes `scoreLocaleMatch` and returns `u32`. |
| `matchLocale` | `(requested: Bcp47Tag, supported: &[]string): string?` | Invokes `matchLocale` and returns `string?`. |

## Minimal Example

```cp
import { isAllAlpha } from "bcp-47";

pub fn main() {
    let result = isAllAlpha("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
