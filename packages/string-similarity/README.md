[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# string-similarity

Zero-allocation Sorensen-Dice bigram string similarity coefficient and best-match finder for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string-similarity` ([https://github.com/aceakash/string-similarity](https://github.com/aceakash/string-similarity))（原库名 `string-similarity` → CP 包名 `string-similarity`）

| Metadata | Value |
| --- | --- |
| Package | `string-similarity` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add string-similarity
```

```cp
import {
    dcToLower,
    dcIsSpace,
    dcNormalize,
    dcEval,
    diceCoefficient,
    diceCaseSensitive,
    bestMatchIndex,
} from "string-similarity";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `dcToLower` | `(c: u8): u8` | Invokes `dcToLower` and returns `u8`. |
| `dcIsSpace` | `(c: u8): bool` | Invokes `dcIsSpace` and returns `bool`. |
| `dcNormalize` | `(src: &[]u8, strip_ws_lower: bool, dst: []u8): usize` | Invokes `dcNormalize` and returns `usize`. |
| `dcEval` | `(a: string, b: string, strip_ws_lower: bool): f64` | Invokes `dcEval` and returns `f64`. |
| `diceCoefficient` | `(a: string, b: string): f64` | Invokes `diceCoefficient` and returns `f64`. |
| `diceCaseSensitive` | `(a: string, b: string): f64` | Invokes `diceCaseSensitive` and returns `f64`. |
| `bestMatchIndex` | `(target: string, candidates: &[]string, min_rating: f64): i64` | Invokes `bestMatchIndex` and returns `i64`. |

## Minimal Example

```cp
import { dcEval } from "string-similarity";

pub fn main() {
    let result = dcEval("example", "example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
