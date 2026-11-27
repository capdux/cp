[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# indoc

Zero-allocation multiline string dedent, indent, stripMargin, and newline normalization engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:indoc` ([https://github.com/dtolnay/indoc](https://github.com/dtolnay/indoc))

| Metadata | Value |
| --- | --- |
| Package | `indoc` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add indoc
```

```cp
import {
    isHws,
    isLineBlank,
    appendByte,
    stripOuterSingleBlankLines,
    trimBlankLines,
    minCommonIndent,
    dedent,
    indent,
} from "indoc";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isHws` | `(c: u8): bool` | Checks predicate `isHws` and returns `bool`. |
| `isLineBlank` | `(s: string, start: usize, end: usize): bool` | Checks predicate `isLineBlank` and returns `bool`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `stripOuterSingleBlankLines` | `(input: string): string` | Invokes `stripOuterSingleBlankLines` and returns `string`. |
| `trimBlankLines` | `(input: string): string` | Invokes `trimBlankLines` and returns `string`. |
| `minCommonIndent` | `(input: string): usize` | Invokes `minCommonIndent` and returns `usize`. |
| `dedent` | `(input: string, out: []u8): string` | Invokes `dedent` and returns `string`. |
| `indent` | `(input: string, prefix: string, skip_empty: bool, out: []u8): string` | Invokes `indent` and returns `string`. |
| `stripMargin` | `(input: string, margin_char: u8, out: []u8): string` | Invokes `stripMargin` and returns `string`. |
| `normalizeNewlines` | `(input: string, out: []u8): string` | Invokes `normalizeNewlines` and returns `string`. |

## Minimal Example

```cp
import { isLineBlank } from "indoc";

pub fn main() {
    let result = isLineBlank("example", 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
