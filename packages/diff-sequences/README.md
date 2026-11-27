[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# diff-sequences

Zero-heap Longest Common Subsequence (LCS) length, diff insert/delete stats, LCS similarity, and Longest Common Substring locator.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:diff-sequences` ([https://github.com/jestjs/jest](https://github.com/jestjs/jest)) / `crates.io:lcs-diff` ([https://github.com/bokuweb/lcs-diff](https://github.com/bokuweb/lcs-diff))（原库名 `diff-sequences` → CP 包名 `diff-sequences`）

| Metadata | Value |
| --- | --- |
| Package | `diff-sequences` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add diff-sequences
```

```cp
import {
    SubstringMatch,
    DiffStats,
    lcsLength,
    diffStats,
    lcsSimilarity,
    longestCommonSubstring,
} from "diff-sequences";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SubstringMatch` | `class` | Fields: `a_start: usize, b_start: usize, len: usize` |
| `DiffStats` | `class` | Fields: `common: usize, insertions: usize, deletions: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lcsLength` | `(a: string, b: string): usize` | Invokes `lcsLength` and returns `usize`. |
| `diffStats` | `(a: string, b: string): DiffStats` | Invokes `diffStats` and returns `DiffStats`. |
| `lcsSimilarity` | `(a: string, b: string): f64` | Invokes `lcsSimilarity` and returns `f64`. |
| `longestCommonSubstring` | `(a: string, b: string): SubstringMatch` | Invokes `longestCommonSubstring` and returns `SubstringMatch`. |

## Minimal Example

```cp
import { lcsLength } from "diff-sequences";

pub fn main() {
    let result = lcsLength("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
