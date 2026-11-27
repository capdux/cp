[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# winnow

Zero-allocation byte-level zero-copy parser combinators (winnow / nom parity) with rollback-on-failure cursor semantics for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:winnow` ([https://github.com/winnow-rs/winnow](https://github.com/winnow-rs/winnow))（原库名 `winnow` → CP 包名 `winnow`）

| Metadata | Value |
| --- | --- |
| Package | `winnow` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add winnow
```

```cp
import {
    Cursor,
    SpanResult,
    IntResult,
    UintResult,
    nbFailSpan,
    tag,
    tagNoCase,
    charLit,
} from "winnow";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Cursor` | `class` | Fields: `data: string, pos: usize` · Methods: `new, remaining, isEmpty, peek, rest, sliceFrom` |
| `SpanResult` | `class` | Fields: `ok: bool, start: usize, len: usize` |
| `IntResult` | `class` | Fields: `ok: bool, value: i64` |
| `UintResult` | `class` | Fields: `ok: bool, value: u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nbFailSpan` | `(start: usize): SpanResult` | Invokes `nbFailSpan` and returns `SpanResult`. |
| `tag` | `(cur: &mut Cursor, lit: string): bool` | Invokes `tag` and returns `bool`. |
| `tagNoCase` | `(cur: &mut Cursor, lit: string): bool` | Invokes `tagNoCase` and returns `bool`. |
| `charLit` | `(cur: &mut Cursor, ch: u8): bool` | Invokes `charLit` and returns `bool`. |
| `take` | `(cur: &mut Cursor, count: usize): SpanResult` | Invokes `take` and returns `SpanResult`. |
| `takeUntil` | `(cur: &mut Cursor, needle: string): SpanResult` | Invokes `takeUntil` and returns `SpanResult`. |
| `takeTillByte` | `(cur: &mut Cursor, stop: u8): SpanResult` | Invokes `takeTillByte` and returns `SpanResult`. |
| `space0` | `(cur: &mut Cursor): SpanResult` | Invokes `space0` and returns `SpanResult`. |
| `multispace0` | `(cur: &mut Cursor): SpanResult` | Invokes `multispace0` and returns `SpanResult`. |
| `digit1` | `(cur: &mut Cursor): SpanResult` | Invokes `digit1` and returns `SpanResult`. |
| `alpha1` | `(cur: &mut Cursor): SpanResult` | Invokes `alpha1` and returns `SpanResult`. |
| `alphanumeric1` | `(cur: &mut Cursor): SpanResult` | Invokes `alphanumeric1` and returns `SpanResult`. |
| `hexDigit1` | `(cur: &mut Cursor): SpanResult` | Invokes `hexDigit1` and returns `SpanResult`. |
| `parseU64` | `(cur: &mut Cursor): UintResult` | Parses input via `parseU64` and returns `UintResult`. |
| `parseI64` | `(cur: &mut Cursor): IntResult` | Parses input via `parseI64` and returns `IntResult`. |
| `delimited` | `(cur: &mut Cursor, open: u8, close: u8): SpanResult` | Invokes `delimited` and returns `SpanResult`. |

## Minimal Example

```cp
import { tag } from "winnow";

pub fn main() {
    let result = tag(/* cur: &mut Cursor */, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
