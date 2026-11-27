[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fastest-levenshtein

Zero-heap Levenshtein edit distance, OSA Damerau-Levenshtein transposition distance, normalized similarity, and closest-match lookup.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fastest-levenshtein` ([https://github.com/ka-weihe/fastest-levenshtein](https://github.com/ka-weihe/fastest-levenshtein)) / `crates.io:levenshtein` ([https://github.com/wooorm/levenshtein-rs](https://github.com/wooorm/levenshtein-rs))（原库名 `fastest-levenshtein` → CP 包名 `fastest-levenshtein`）

| Metadata | Value |
| --- | --- |
| Package | `fastest-levenshtein` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fastest-levenshtein
```

```cp
import {
    absDiff,
    min3,
    distance,
    damerauDistance,
    similarity,
    withinDistance,
    closestIndex,
} from "fastest-levenshtein";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absDiff` | `(a: usize, b: usize): usize` | Invokes `absDiff` and returns `usize`. |
| `min3` | `(a: usize, b: usize, c: usize): usize` | Invokes `min3` and returns `usize`. |
| `distance` | `(a: string, b: string): usize` | Invokes `distance` and returns `usize`. |
| `damerauDistance` | `(a: string, b: string): usize` | Invokes `damerauDistance` and returns `usize`. |
| `similarity` | `(a: string, b: string): f64` | Invokes `similarity` and returns `f64`. |
| `withinDistance` | `(a: string, b: string, max_dist: usize): bool` | Invokes `withinDistance` and returns `bool`. |
| `closestIndex` | `(query: string, candidates: &[]string, max_dist: usize): i64` | Releases native resources allocated by this module. |

## Minimal Example

```cp
import { distance } from "fastest-levenshtein";

pub fn main() {
    let result = distance("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
