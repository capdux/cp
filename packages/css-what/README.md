[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# css-what

Zero-allocation CSS selector tokenizer, parser, and serializer (Cheerio / css-select core) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:css-what` ([https://github.com/fb55/css-what](https://github.com/fb55/css-what))

| Metadata | Value |
| --- | --- |
| Package | `css-what` |
| Version | `1.0.0` |
| License | `BSD-2-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add css-what
```

```cp
import {
    CssTokenKind,
    AttrAction,
    CssSelectorToken,
    CwhWriter,
    cwhIsIdentStart,
    cwhIsIdentChar,
    cwhSkipSpace,
    cwhScanIdent,
} from "css-what";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CssTokenKind` | `Tag, Universal, Id, Class, Attribute, Pseudo, PseudoElement, Descendant ...` |
| `enum` | `AttrAction` | `Exists, Equals, Start, End, Any, Not, Element, Hyphen` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CssSelectorToken` | `class` | Fields: `kind: CssTokenKind, name: string, value: string, action: AttrAction, ignore_case: bool` |
| `CwhWriter` | `class` | Fields: `buf: []u8, pos: usize, overflow: bool` · Methods: `writeByte, putStr` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cwhIsIdentStart` | `(c: u8): bool` | Invokes `cwhIsIdentStart` and returns `bool`. |
| `cwhIsIdentChar` | `(c: u8): bool` | Invokes `cwhIsIdentChar` and returns `bool`. |
| `cwhSkipSpace` | `(s: string, start: usize): usize` | Invokes `cwhSkipSpace` and returns `usize`. |
| `cwhScanIdent` | `(s: string, start: usize): usize` | Invokes `cwhScanIdent` and returns `usize`. |
| `cwhIsCombinatorChar` | `(c: u8): bool` | Invokes `cwhIsCombinatorChar` and returns `bool`. |
| `cwhToken` | `(kind: CssTokenKind, name: string, value: string, action: AttrAction, ignore_case: bool): CssSelectorToken` | Invokes `cwhToken` and returns `CssSelectorToken`. |
| `parseCssSelector` | `(selector: string, out: []CssSelectorToken): i64` | Parses input via `parseCssSelector` and returns `i64`. |
| `stringifyCssSelector` | `(tokens: &[]CssSelectorToken, out: []u8): usize` | Serializes/formats data via `stringifyCssSelector` returning `usize`. |

## Minimal Example

```cp
import { parseCssSelector } from "css-what";

pub fn main() {
    let result = parseCssSelector("example", /* out: []CssSelectorToken */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
