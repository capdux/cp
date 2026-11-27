[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# serde_qs

Zero-allocation nested bracket query string parser & serializer (crates.io serde_qs v0.13 parity) composing percent-encoding for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:serde_qs` ([https://github.com/samscott89/serde_qs](https://github.com/samscott89/serde_qs))（原库名 `serde_qs` → CP 包名 `serde_qs`）

| Metadata | Value |
| --- | --- |
| Package | `serde_qs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add serde_qs
```

```cp
import {
    QsPair,
    QsPath,
    sqsKeyEncodeSet,
    parseKeyPath,
    parseQs,
    getQsValueDecoded,
    serializeQs,
} from "serde_qs";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `QsPair` | `class` | Fields: `key: string, value: string` |
| `QsPath` | `class` | Fields: `segments: [8]string, depth: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sqsKeyEncodeSet` | `(): AsciiSet` | Invokes `sqsKeyEncodeSet` and returns `AsciiSet`. |
| `parseKeyPath` | `(key: string): QsPath` | Parses input via `parseKeyPath` and returns `QsPath`. |
| `parseQs` | `(query: string, out_pairs: []QsPair): usize` | Parses input via `parseQs` and returns `usize`. |
| `getQsValueDecoded` | `(query: string, target_key: string, out: []u8): usize?` | Queries/retrieves data via `getQsValueDecoded` returning `usize?`. |
| `serializeQs` | `(pairs: &[]QsPair, out: []u8): usize` | Invokes `serializeQs` and returns `usize`. |

## Minimal Example

```cp
import { parseKeyPath } from "serde_qs";

pub fn main() {
    let result = parseKeyPath("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
