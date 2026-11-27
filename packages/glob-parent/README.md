[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# glob-parent

Zero-allocation glob static parent directory extractor, glob detector, and escaper (glob-parent + is-glob parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:glob-parent` ([https://github.com/gulpjs/glob-parent](https://github.com/gulpjs/glob-parent))

| Metadata | Value |
| --- | --- |
| Package | `glob-parent` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add glob-parent
```

```cp
import {
    GlobSplit,
    findMatchingBracket,
    findMatchingBrace,
    findFirstGlobPos,
    stripLeadingNegation,
    trimTrailingSlashes,
    isGlob,
    globParent,
} from "glob-parent";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GlobSplit` | `class` | Fields: `base: string, glob: string, is_glob: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `findMatchingBracket` | `(s: string, openIdx: usize): usize?` | Queries/retrieves data via `findMatchingBracket` returning `usize?`. |
| `findMatchingBrace` | `(s: string, openIdx: usize): usize?` | Queries/retrieves data via `findMatchingBrace` returning `usize?`. |
| `findFirstGlobPos` | `(s: string): usize?` | Queries/retrieves data via `findFirstGlobPos` returning `usize?`. |
| `stripLeadingNegation` | `(raw: string): string` | Invokes `stripLeadingNegation` and returns `string`. |
| `trimTrailingSlashes` | `(s: string): string` | Invokes `trimTrailingSlashes` and returns `string`. |
| `isGlob` | `(pattern: string): bool` | Checks predicate `isGlob` and returns `bool`. |
| `globParent` | `(pattern: string): string` | Invokes `globParent` and returns `string`. |
| `splitGlobBase` | `(pattern: string): GlobSplit` | Invokes `splitGlobBase` and returns `GlobSplit`. |
| `isGlobSpecial` | `(c: u8): bool` | Checks predicate `isGlobSpecial` and returns `bool`. |
| `escapeGlob` | `(raw: string, out: []u8): string` | Invokes `escapeGlob` and returns `string`. |

## Minimal Example

```cp
import { isGlob } from "glob-parent";

pub fn main() {
    let result = isGlob("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
