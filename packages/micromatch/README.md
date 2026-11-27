[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# micromatch

Zero-allocation glob matching suite (npm micromatch v4 parity) with brace range expansion, multi-pattern filtering, negation, and contains matching for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:micromatch` ([https://github.com/micromatch/micromatch](https://github.com/micromatch/micromatch))

| Metadata | Value |
| --- | --- |
| Package | `micromatch` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add micromatch
```

```cp
import {
    mmHasBrace,
    mmCountLeadingNeg,
    mmMatchPositiveExpanded,
    mmWriteWrapped,
    mmContainsSinglePositive,
    mmIsMatch,
    mmAll,
    mmAny,
} from "micromatch";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mmHasBrace` | `(s: string): bool` | Invokes `mmHasBrace` and returns `bool`. |
| `mmCountLeadingNeg` | `(pat: string): usize` | Invokes `mmCountLeadingNeg` and returns `usize`. |
| `mmMatchPositiveExpanded` | `(path: string, inner: string, opts: PicoOptions): bool` | Invokes `mmMatchPositiveExpanded` and returns `bool`. |
| `mmWriteWrapped` | `(dst: []u8, prefix: string, mid: string, suffix: string): usize` | Invokes `mmWriteWrapped` and returns `usize`. |
| `mmContainsSinglePositive` | `(path: string, pat: string, opts: PicoOptions): bool` | Invokes `mmContainsSinglePositive` and returns `bool`. |
| `mmIsMatch` | `(path: string, pattern: string, opts: PicoOptions): bool` | Invokes `mmIsMatch` and returns `bool`. |
| `mmAll` | `(path: string, patterns: &[]string, opts: PicoOptions): bool` | Invokes `mmAll` and returns `bool`. |
| `mmAny` | `(path: string, patterns: &[]string, opts: PicoOptions): bool` | Invokes `mmAny` and returns `bool`. |
| `mmSome` | `(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool` | Invokes `mmSome` and returns `bool`. |
| `mmEvery` | `(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool` | Invokes `mmEvery` and returns `bool`. |
| `mmFilter` | `(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize` | Invokes `mmFilter` and returns `usize`. |
| `mmNot` | `(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize` | Invokes `mmNot` and returns `usize`. |
| `mmContains` | `(path: string, pattern: string, opts: PicoOptions): bool` | Invokes `mmContains` and returns `bool`. |

## Minimal Example

```cp
import { mmHasBrace } from "micromatch";

pub fn main() {
    let result = mmHasBrace("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
