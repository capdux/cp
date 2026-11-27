[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# strsim

Zero-heap string similarity & edit-distance metrics: Hamming, Levenshtein, OSA Damerau-Levenshtein, Jaro-Winkler, and Sorensen-Dice (clean-room port of Rust strsim)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:strsim` ([https://github.com/rapidfuzz/strsim-rs](https://github.com/rapidfuzz/strsim-rs))

| Metadata | Value |
| --- | --- |
| Package | `strsim` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add strsim
```

```cp
import {
    ssmMin3,
    hamming,
    levenshtein,
    osaDistance,
    normalizedLevenshteinBps,
    jaroBps,
    jaroWinklerBps,
    sorensenDiceBps,
} from "strsim";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ssmMin3` | `(a: usize, b: usize, c: usize): usize` | Invokes `ssmMin3` and returns `usize`. |
| `hamming` | `(a: string, b: string): usize?` | Invokes `hamming` and returns `usize?`. |
| `levenshtein` | `(a: string, b: string): usize` | Invokes `levenshtein` and returns `usize`. |
| `osaDistance` | `(a: string, b: string): usize` | Invokes `osaDistance` and returns `usize`. |
| `normalizedLevenshteinBps` | `(a: string, b: string): u32` | Invokes `normalizedLevenshteinBps` and returns `u32`. |
| `jaroBps` | `(a: string, b: string): u32` | Invokes `jaroBps` and returns `u32`. |
| `jaroWinklerBps` | `(a: string, b: string): u32` | Invokes `jaroWinklerBps` and returns `u32`. |
| `sorensenDiceBps` | `(a: string, b: string): u32` | Invokes `sorensenDiceBps` and returns `u32`. |

## Minimal Example

```cp
import { hamming } from "strsim";

pub fn main() {
    let result = hamming("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
