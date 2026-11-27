[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# didyoumean

Zero-allocation spell-suggestion & edit-distance engine (didyoumean / fastest-levenshtein / Rust strsim parity: Levenshtein, Damerau-Levenshtein, Jaro-Winkler & CLI command suggestion) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:didyoumean` ([https://github.com/dcporter/didyoumean.js](https://github.com/dcporter/didyoumean.js))

| Metadata | Value |
| --- | --- |
| Package | `didyoumean` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add didyoumean
```

```cp
import {
    Suggestion,
    min3,
    levenshtein,
    damerauLevenshtein,
    similarity,
    didYouMeanWith,
    didYouMean,
} from "didyoumean";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Suggestion` | `class` | Fields: `found: bool, match_str: string, distance: usize, index: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `min3` | `(a: usize, b: usize, c: usize): usize` | Invokes `min3` and returns `usize`. |
| `levenshtein` | `(a: string, b: string): usize` | Invokes `levenshtein` and returns `usize`. |
| `damerauLevenshtein` | `(a: string, b: string, ignore_case: bool): usize` | Invokes `damerauLevenshtein` and returns `usize`. |
| `similarity` | `(a: string, b: string): f64` | Invokes `similarity` and returns `f64`. |
| `didYouMeanWith` | `(input: string, candidates: []string, max_dist: usize): Suggestion` | Invokes `didYouMeanWith` and returns `Suggestion`. |
| `didYouMean` | `(input: string, candidates: []string): string?` | Invokes `didYouMean` and returns `string?`. |

## Minimal Example

```cp
import { levenshtein } from "didyoumean";

pub fn main() {
    let result = levenshtein("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
