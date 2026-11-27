[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# combine

Zero-copy streaming parser combinators with line/column cursor tracking and backtracking (Rust combine v4 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:combine` ([https://github.com/Marwes/combine](https://github.com/Marwes/combine))

| Metadata | Value |
| --- | --- |
| Package | `combine` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add combine
```

```cp
import {
    CharPred,
    CursorMark,
    ParseCursor,
    cmbIsAlpha,
    cmbIsDigit,
    cmbIsSpace,
    cmbIsHex,
    cmbMatchPred,
} from "combine";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CharPred` | `Alpha, Digit, AlphaNum, Space, HexDigit, Ident, NonSpace` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CursorMark` | `class` | Fields: `pos: usize, line: usize, col: usize` |
| `ParseCursor` | `class` | Fields: `input: string, pos: usize, line: usize, col: usize` · Methods: `new, remaining, isEof, peek, advance, save, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cmbIsAlpha` | `(ch: u8): bool` | Invokes `cmbIsAlpha` and returns `bool`. |
| `cmbIsDigit` | `(ch: u8): bool` | Invokes `cmbIsDigit` and returns `bool`. |
| `cmbIsSpace` | `(ch: u8): bool` | Invokes `cmbIsSpace` and returns `bool`. |
| `cmbIsHex` | `(ch: u8): bool` | Invokes `cmbIsHex` and returns `bool`. |
| `cmbMatchPred` | `(ch: u8, kind: CharPred): bool` | Invokes `cmbMatchPred` and returns `bool`. |
| `cmbStartsWithAt` | `(hay: string, pos: usize, needle: string): bool` | Invokes `cmbStartsWithAt` and returns `bool`. |
| `tag` | `(cursor: &mut ParseCursor, lit: string): bool` | Invokes `tag` and returns `bool`. |
| `charByte` | `(cursor: &mut ParseCursor, expected: u8): bool` | Invokes `charByte` and returns `bool`. |
| `takeWhile` | `(cursor: &mut ParseCursor, pred_kind: CharPred): string` | Invokes `takeWhile` and returns `string`. |
| `takeWhile1` | `(cursor: &mut ParseCursor, pred_kind: CharPred): string?` | Invokes `takeWhile1` and returns `string?`. |
| `takeUntil` | `(cursor: &mut ParseCursor, delim: string): string?` | Invokes `takeUntil` and returns `string?`. |
| `skipSpaces` | `(cursor: &mut ParseCursor): usize` | Invokes `skipSpaces` and returns `usize`. |
| `parseU64` | `(cursor: &mut ParseCursor): u64?` | Parses input via `parseU64` and returns `u64?`. |
| `parseI64` | `(cursor: &mut ParseCursor): i64?` | Parses input via `parseI64` and returns `i64?`. |
| `parseQuotedString` | `(cursor: &mut ParseCursor): string?` | Parses input via `parseQuotedString` and returns `string?`. |
| `delimited` | `(cursor: &mut ParseCursor, open: string, close: string): string?` | Invokes `delimited` and returns `string?`. |
| `separatedList` | `(cursor: &mut ParseCursor, sep: string, out: []string): usize` | Invokes `separatedList` and returns `usize`. |

## Minimal Example

```cp
import { cmbStartsWithAt } from "combine";

pub fn main() {
    let result = cmbStartsWithAt("example", 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
