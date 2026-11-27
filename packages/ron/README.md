[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ron

Zero-heap Rusty Object Notation (RON) lexer and top-level field extractor in pure CP

| Metadata | Value |
| --- | --- |
| Package | `ron` |
| Version | `0.8.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ron
```

```cp
import {
    RonTokenKind,
    RonToken,
    RonLexer,
    ronpIsWs,
    ronpIsDigit,
    ronpIsHexDigit,
    ronpIsIdentStart,
    ronpIsIdentCont,
} from "ron";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `RonTokenKind` | `Ident, String, Int, Float, Bool, LParen, RParen, LBracket ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RonToken` | `class` | Fields: `kind: RonTokenKind, text: string, start: usize, end: usize` |
| `RonLexer` | `class` | Fields: `src: string, pos: usize` · Methods: `new, nextToken` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ronpIsWs` | `(c: u8): bool` | Invokes `ronpIsWs` and returns `bool`. |
| `ronpIsDigit` | `(c: u8): bool` | Invokes `ronpIsDigit` and returns `bool`. |
| `ronpIsHexDigit` | `(c: u8): bool` | Invokes `ronpIsHexDigit` and returns `bool`. |
| `ronpIsIdentStart` | `(c: u8): bool` | Invokes `ronpIsIdentStart` and returns `bool`. |
| `ronpIsIdentCont` | `(c: u8): bool` | Invokes `ronpIsIdentCont` and returns `bool`. |
| `ronpEq` | `(a: string, b: string): bool` | Invokes `ronpEq` and returns `bool`. |
| `ronpSkipTrivia` | `(src: string, start_pos: usize): usize` | Invokes `ronpSkipTrivia` and returns `usize`. |
| `ronpIsOpenDelim` | `(k: RonTokenKind): bool` | Invokes `ronpIsOpenDelim` and returns `bool`. |
| `ronpIsCloseDelim` | `(k: RonTokenKind): bool` | Invokes `ronpIsCloseDelim` and returns `bool`. |
| `ronGetField` | `(ron_src: string, field_name: string): string?` | Invokes `ronGetField` and returns `string?`. |

## Minimal Example

```cp
import { ronpEq } from "ron";

pub fn main() {
    let result = ronpEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
