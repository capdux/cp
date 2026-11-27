[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# abbrev

Zero-heap unique unambiguous prefix abbreviation generator and resolver for CP (clean-room ISC implementation of npm abbrev).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:abbrev` ([https://github.com/npm/abbrev-js](https://github.com/npm/abbrev-js))（原库名 `abbrev` → CP 包名 `abbrev`）

| Metadata | Value |
| --- | --- |
| Package | `abbrev` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add abbrev
```

```cp
import {
    AbbrevEntry,
    abvStrEq,
    abvStrLt,
    abvHasPrefix,
    abvLcp,
    abbrev,
    shortestAbbrev,
    resolveAbbrev,
} from "abbrev";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AbbrevEntry` | `class` | Fields: `abbrev: string, full: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `abvStrEq` | `(a: string, b: string): bool` | Invokes `abvStrEq` and returns `bool`. |
| `abvStrLt` | `(a: string, b: string): bool` | Invokes `abvStrLt` and returns `bool`. |
| `abvHasPrefix` | `(word: string, prefix: string): bool` | Invokes `abvHasPrefix` and returns `bool`. |
| `abvLcp` | `(a: string, b: string): usize` | Invokes `abvLcp` and returns `usize`. |
| `abbrev` | `(words: &[]string, out: []AbbrevEntry): usize` | Invokes `abbrev` and returns `usize`. |
| `shortestAbbrev` | `(words: &[]string, target: string): string?` | Invokes `shortestAbbrev` and returns `string?`. |
| `resolveAbbrev` | `(words: &[]string, prefix: string): string?` | Invokes `resolveAbbrev` and returns `string?`. |

## Minimal Example

```cp
import { abvStrEq } from "abbrev";

pub fn main() {
    let result = abvStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
