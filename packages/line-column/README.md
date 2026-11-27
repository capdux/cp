[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# line-column

Zero-allocation byte offset <-> 1-based (line, col) locator, zero-copy line extractor, and LF/CRLF line counter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:line-column` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:line-index` ([https://github.com/rust-lang/rust-analyzer](https://github.com/rust-lang/rust-analyzer))（原库名 `line-column` → CP 包名 `line-column`）

| Metadata | Value |
| --- | --- |
| Package | `line-column` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add line-column
```

```cp
import {
    LineCol,
    offsetToLineCol,
    lineColToOffset,
    getLine,
    countLines,
} from "line-column";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LineCol` | `class` | Fields: `line: usize, col: usize, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `offsetToLineCol` | `(text: string, offset: usize): LineCol` | Invokes `offsetToLineCol` and returns `LineCol`. |
| `lineColToOffset` | `(text: string, line: usize, col: usize): i64` | Invokes `lineColToOffset` and returns `i64`. |
| `getLine` | `(text: string, line_1based: usize): string` | Queries/retrieves data via `getLine` returning `string`. |
| `countLines` | `(text: string): usize` | Invokes `countLines` and returns `usize`. |

## Minimal Example

```cp
import { getLine } from "line-column";

pub fn main() {
    let result = getLine("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
