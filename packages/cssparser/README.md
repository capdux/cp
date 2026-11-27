[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cssparser

Zero-heap CSS Syntax Level 3 tokenizer and lexer in pure CP

| Metadata | Value |
| --- | --- |
| Package | `cssparser` |
| Version | `0.34.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cssparser
```

```cp
import {
    CssTokKind,
    CssToken,
    CssLexer,
    csspIsIdentStart,
    csspIsIdentChar,
    csspStartsIdent,
    csspStartsNumber,
    csspScanIdent,
} from "cssparser";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CssTokKind` | `Ident, AtKeyword, Hash, String, Number, Percentage, Dimension, Function ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CssToken` | `class` | Fields: `kind: CssTokKind, text: string, unit: string, start: usize, end: usize` |
| `CssLexer` | `class` | Fields: `src: string, pos: usize` · Methods: `new, nextToken` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `csspIsIdentStart` | `(c: u8): bool` | Invokes `csspIsIdentStart` and returns `bool`. |
| `csspIsIdentChar` | `(c: u8): bool` | Invokes `csspIsIdentChar` and returns `bool`. |
| `csspStartsIdent` | `(s: string, pos: usize): bool` | Invokes `csspStartsIdent` and returns `bool`. |
| `csspStartsNumber` | `(s: string, pos: usize): bool` | Invokes `csspStartsNumber` and returns `bool`. |
| `csspScanIdent` | `(s: string, start: usize): usize` | Invokes `csspScanIdent` and returns `usize`. |
| `csspScanNumber` | `(s: string, start: usize): usize` | Invokes `csspScanNumber` and returns `usize`. |
| `csspSkipWsAndComments` | `(s: string, start: usize): usize` | Invokes `csspSkipWsAndComments` and returns `usize`. |

## Minimal Example

```cp
import { csspStartsIdent } from "cssparser";

pub fn main() {
    let result = csspStartsIdent("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
