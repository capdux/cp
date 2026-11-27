[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jaro-winkler

Zero-allocation Jaro and Jaro-Winkler string similarity metrics and best-match finder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jaro-winkler` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:jaro-winkler` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `jaro-winkler` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add jaro-winkler
```

```cp
import { jaro, jaroWinklerWithScale, jaroWinkler, bestMatchIndex } from "jaro-winkler";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `jaro` | `(a: string, b: string): f64` | Invokes `jaro` and returns `f64`. |
| `jaroWinklerWithScale` | `(a: string, b: string, prefix_scale: f64): f64` | Invokes `jaroWinklerWithScale` and returns `f64`. |
| `jaroWinkler` | `(a: string, b: string): f64` | Invokes `jaroWinkler` and returns `f64`. |
| `bestMatchIndex` | `(query: string, candidates: &[]string, min_score: f64): i64` | Invokes `bestMatchIndex` and returns `i64`. |

## Minimal Example

```cp
import { jaro } from "jaro-winkler";

pub fn main() {
    let result = jaro("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
