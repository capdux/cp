[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @paralleldrive/cuid2

Zero-heap collision-resistant, URL-safe, lowercase CUID2 ID generator and validator (clean-room port of npm @paralleldrive/cuid2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@paralleldrive/cuid2` ([https://github.com/paralleldrive/cuid2](https://github.com/paralleldrive/cuid2))（原库名 `@paralleldrive/cuid2` → CP 包名 `@paralleldrive/cuid2`）

| Metadata | Value |
| --- | --- |
| Package | `@paralleldrive/cuid2` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @paralleldrive/cuid2
```

```cp
import {
    Cuid2Generator,
    c2Mix,
    c2Base36Char,
    c2LetterChar,
    isCuid2,
    isCuid2WithBounds,
} from "@paralleldrive/cuid2";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Cuid2Generator` | `class` | Fields: `state: u64, counter: u64, fingerprint_hash: u64, id_len: usize` · Methods: `new, length, next` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `c2Mix` | `(x: u64): u64` | Invokes `c2Mix` and returns `u64`. |
| `c2Base36Char` | `(v: u64): u8` | Invokes `c2Base36Char` and returns `u8`. |
| `c2LetterChar` | `(v: u64): u8` | Invokes `c2LetterChar` and returns `u8`. |
| `isCuid2` | `(id: string): bool` | Checks predicate `isCuid2` and returns `bool`. |
| `isCuid2WithBounds` | `(id: string, min_len: usize, max_len: usize): bool` | Checks predicate `isCuid2WithBounds` and returns `bool`. |

## Minimal Example

```cp
import { isCuid2 } from "@paralleldrive/cuid2";

pub fn main() {
    let result = isCuid2("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
