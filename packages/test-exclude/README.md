[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# test-exclude

Zero-heap test and coverage file path glob filter engine

| Metadata | Value |
| --- | --- |
| Package | `test-exclude` |
| Version | `7.0.1` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add test-exclude
```

```cp
import {
    ExcludeConfig,
    texcHasPrefix,
    texcHasSuffix,
    texcContainsSegment,
    texcMatchAt,
    texcMatchRaw,
    matchSimpleGlob,
    texcMatchesExt,
} from "test-exclude";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ExcludeConfig` | `class` | Fields: `includes: &[]string, excludes: &[]string, extensions: &[]string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `texcHasPrefix` | `(s: string, prefix: string): bool` | Invokes `texcHasPrefix` and returns `bool`. |
| `texcHasSuffix` | `(s: string, suffix: string): bool` | Invokes `texcHasSuffix` and returns `bool`. |
| `texcContainsSegment` | `(path: string, seg: string): bool` | Invokes `texcContainsSegment` and returns `bool`. |
| `texcMatchAt` | `(pat: string, pi: usize, text: string, ti: usize): bool` | Invokes `texcMatchAt` and returns `bool`. |
| `texcMatchRaw` | `(pat: string, path: string): bool` | Invokes `texcMatchRaw` and returns `bool`. |
| `matchSimpleGlob` | `(pattern: string, path: string): bool` | Invokes `matchSimpleGlob` and returns `bool`. |
| `texcMatchesExt` | `(path: string, ext: string): bool` | Invokes `texcMatchesExt` and returns `bool`. |
| `shouldInstrument` | `(rel_path: string, cfg: ExcludeConfig): bool` | Invokes `shouldInstrument` and returns `bool`. |

## Minimal Example

```cp
import { texcHasPrefix } from "test-exclude";

pub fn main() {
    let result = texcHasPrefix("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
