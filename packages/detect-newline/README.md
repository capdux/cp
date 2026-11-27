[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# detect-newline

Zero-allocation dominant newline detector and cross-platform LF/CRLF normalizer for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:detect-newline` ([https://github.com/sindresorhus/detect-newline](https://github.com/sindresorhus/detect-newline))

| Metadata | Value |
| --- | --- |
| Package | `detect-newline` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add detect-newline
```

```cp
import {
    NewlineStats,
    analyzeNewlines,
    detectNewline,
    detectNewlineGraceful,
    normalizeToLf,
    normalizeToCrlf,
} from "detect-newline";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `NewlineStats` | `class` | Fields: `lf_count: usize, crlf_count: usize, cr_count: usize, dominant: u8` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `analyzeNewlines` | `(input: string): NewlineStats` | Invokes `analyzeNewlines` and returns `NewlineStats`. |
| `detectNewline` | `(input: string): string` | Invokes `detectNewline` and returns `string`. |
| `detectNewlineGraceful` | `(input: string): string` | Invokes `detectNewlineGraceful` and returns `string`. |
| `normalizeToLf` | `(input: string, out: []u8): usize` | Invokes `normalizeToLf` and returns `usize`. |
| `normalizeToCrlf` | `(input: string, out: []u8): usize` | Invokes `normalizeToCrlf` and returns `usize`. |

## Minimal Example

```cp
import { analyzeNewlines } from "detect-newline";

pub fn main() {
    let result = analyzeNewlines("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
