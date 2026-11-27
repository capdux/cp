[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unicode-linebreak

Zero-allocation pure CP UAX #14 Unicode line break opportunity detector (unicode-linebreak parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-linebreak` ([https://github.com/axelf4/unicode-linebreak](https://github.com/axelf4/unicode-linebreak))

| Metadata | Value |
| --- | --- |
| Package | `unicode-linebreak` |
| Version | `1.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unicode-linebreak
```

```cp
import {
    BreakKind,
    LineBreak,
    UlbrDecoded,
    ulbrDecodeUtf8,
    ulbrClassify,
    ulbrEvalBreak,
    findLineBreaks,
    countLineBreaks,
} from "unicode-linebreak";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `BreakKind` | `Mandatory, Allowed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LineBreak` | `class` | Fields: `offset: usize, kind: BreakKind` |
| `UlbrDecoded` | `class` | Fields: `cp: u32, len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ulbrDecodeUtf8` | `(s: string, pos: usize): UlbrDecoded` | Invokes `ulbrDecodeUtf8` and returns `UlbrDecoded`. |
| `ulbrClassify` | `(cp: u32): u8` | Invokes `ulbrClassify` and returns `u8`. |
| `ulbrEvalBreak` | `(prev: u8, prevNonSp: u8, curr: u8): u8` | Invokes `ulbrEvalBreak` and returns `u8`. |
| `findLineBreaks` | `(utf8: string, out: []LineBreak): usize` | Queries/retrieves data via `findLineBreaks` returning `usize`. |
| `countLineBreaks` | `(utf8: string): usize` | Invokes `countLineBreaks` and returns `usize`. |

## Minimal Example

```cp
import { ulbrDecodeUtf8 } from "unicode-linebreak";

pub fn main() {
    let result = ulbrDecodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
