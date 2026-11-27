[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# slugify

Zero-allocation URL slug generator with UTF-8 Latin-1 / Latin Extended-A diacritic folding and truncation for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`slugify`](https://github.com/simov/slugify)（`MIT`）（包名 `slugify`）。

| Metadata | Value |
| --- | --- |
| Package | `slugify` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add slugify
```

```cp
import {
    slToLower,
    slIsAsciiAlnum,
    slFoldLatin,
    slugifyWithSep,
    slugify,
    slugifyTruncate,
    isValidSlug,
} from "slugify";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `slToLower` | `(c: u8): u8` | Invokes `slToLower` and returns `u8`. |
| `slIsAsciiAlnum` | `(c: u8): bool` | Invokes `slIsAsciiAlnum` and returns `bool`. |
| `slFoldLatin` | `(cp: u32, lowercase: bool, buf: []u8): usize` | Invokes `slFoldLatin` and returns `usize`. |
| `slugifyWithSep` | `(input: string, sep: u8, lowercase: bool, out: []u8): usize` | Invokes `slugifyWithSep` and returns `usize`. |
| `slugify` | `(input: string, out: []u8): usize` | Invokes `slugify` and returns `usize`. |
| `slugifyTruncate` | `(input: string, max_len: usize, out: []u8): usize` | Invokes `slugifyTruncate` and returns `usize`. |
| `isValidSlug` | `(s: string): bool` | Checks predicate `isValidSlug` and returns `bool`. |

## Minimal Example

```cp
import { isValidSlug } from "slugify";

pub fn main() {
    let result = isValidSlug("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
