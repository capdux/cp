[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# css-select

Zero-heap CSS selector parser, W3C Selectors Level 4 specificity calculator, and compound/list element matcher (clean-room port of css-select + css-what)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:css-select` ([https://github.com/fb55/css-select](https://github.com/fb55/css-select))

| Metadata | Value |
| --- | --- |
| Package | `css-select` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add css-select
```

```cp
import {
    Specificity,
    CssElement,
    csssIsCombinator,
    csssIsIdentStart,
    csssIsIdentChar,
    csssEqMaybeCi,
    csssSkipIdent,
    csssFindBracketEnd,
} from "css-select";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Specificity` | `class` | Fields: `ids: u32, classes: u32, tags: u32` · Methods: `new, cmp, toScore` |
| `CssElement` | `class` | Fields: `tag: string, id: string, class_list: string, attr_name: string, attr_val: string` · Methods: `new` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `csssIsCombinator` | `(c: u8): bool` | Invokes `csssIsCombinator` and returns `bool`. |
| `csssIsIdentStart` | `(c: u8): bool` | Invokes `csssIsIdentStart` and returns `bool`. |
| `csssIsIdentChar` | `(c: u8): bool` | Invokes `csssIsIdentChar` and returns `bool`. |
| `csssEqMaybeCi` | `(a: string, b: string, ci: bool): bool` | Invokes `csssEqMaybeCi` and returns `bool`. |
| `csssSkipIdent` | `(s: string, pos: usize): usize` | Invokes `csssSkipIdent` and returns `usize`. |
| `csssFindBracketEnd` | `(s: string, pos: usize): usize` | Invokes `csssFindBracketEnd` and returns `usize`. |
| `csssSkipParen` | `(s: string, pos: usize): usize` | Invokes `csssSkipParen` and returns `usize`. |
| `csssHasToken` | `(list: string, target: string, ci: bool): bool` | Invokes `csssHasToken` and returns `bool`. |
| `csssContains` | `(haystack: string, needle: string, ci: bool): bool` | Invokes `csssContains` and returns `bool`. |
| `csssIsLegacyPseudoElement` | `(name: string): bool` | Invokes `csssIsLegacyPseudoElement` and returns `bool`. |
| `csssComputeSingleSpecificity` | `(s: string, depth: u32): Specificity` | Invokes `csssComputeSingleSpecificity` and returns `Specificity`. |
| `csssComputeListSpecificity` | `(selector: string, depth: u32): Specificity` | Invokes `csssComputeListSpecificity` and returns `Specificity`. |
| `computeSpecificity` | `(selector: string): Specificity` | Invokes `computeSpecificity` and returns `Specificity`. |
| `csssMatchAttr` | `(raw_inner: string, el: CssElement): bool` | Invokes `csssMatchAttr` and returns `bool`. |
| `matchesCompoundSelector` | `(selector: string, el: CssElement): bool` | Invokes `matchesCompoundSelector` and returns `bool`. |
| `matchesSelectorList` | `(selector_list: string, el: CssElement): bool` | Invokes `matchesSelectorList` and returns `bool`. |
| `countSelectorParts` | `(selector: string): usize` | Invokes `countSelectorParts` and returns `usize`. |

## Minimal Example

```cp
import { csssEqMaybeCi } from "css-select";

pub fn main() {
    let result = csssEqMaybeCi("example", "example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
