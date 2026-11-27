[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# language-tags

Zero-heap BCP 47 / RFC 5646 language tag parser, canonical formatter, and RFC 4647 range matcher in pure CP

| Metadata | Value |
| --- | --- |
| Package | `language-tags` |
| Version | `1.0.9` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add language-tags
```

```cp
import {
    Bcp47Tag,
    ltgIsAlpha,
    ltgIsDigit,
    ltgIsAlnum,
    ltgIsSep,
    ltgToLower,
    ltgToUpper,
    ltgAllAlpha,
} from "language-tags";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Bcp47Tag` | `class` | Fields: `language: string, script: string, region: string, variant: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ltgIsAlpha` | `(c: u8): bool` | Invokes `ltgIsAlpha` and returns `bool`. |
| `ltgIsDigit` | `(c: u8): bool` | Invokes `ltgIsDigit` and returns `bool`. |
| `ltgIsAlnum` | `(c: u8): bool` | Invokes `ltgIsAlnum` and returns `bool`. |
| `ltgIsSep` | `(c: u8): bool` | Invokes `ltgIsSep` and returns `bool`. |
| `ltgToLower` | `(c: u8): u8` | Invokes `ltgToLower` and returns `u8`. |
| `ltgToUpper` | `(c: u8): u8` | Invokes `ltgToUpper` and returns `u8`. |
| `ltgAllAlpha` | `(s: string): bool` | Invokes `ltgAllAlpha` and returns `bool`. |
| `ltgAllDigit` | `(s: string): bool` | Invokes `ltgAllDigit` and returns `bool`. |
| `ltgAllAlnum` | `(s: string): bool` | Invokes `ltgAllAlnum` and returns `bool`. |
| `ltgInvalidTag` | `(): Bcp47Tag` | Invokes `ltgInvalidTag` and returns `Bcp47Tag`. |
| `parseLanguageTag` | `(tag: string): Bcp47Tag` | Parses input via `parseLanguageTag` and returns `Bcp47Tag`. |
| `ltgPushByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `ltgPushByte` and returns `usize`. |
| `formatCanonicalTag` | `(t: Bcp47Tag, out: []u8): usize` | Serializes/formats data via `formatCanonicalTag` returning `usize`. |
| `matchesLanguageRange` | `(tag: string, range: string): bool` | Invokes `matchesLanguageRange` and returns `bool`. |

## Minimal Example

```cp
import { parseLanguageTag } from "language-tags";

pub fn main() {
    let result = parseLanguageTag("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
