[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# is-glob

Zero-allocation glob pattern detector reusing is-extglob for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-glob` ([https://github.com/micromatch/is-glob](https://github.com/micromatch/is-glob))

| Metadata | Value |
| --- | --- |
| Package | `is-glob` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add is-glob
```

```cp
import {
    hasClosedBracket,
    hasClosedBrace,
    hasUnescapedWildcard,
    isGlob,
    isGlobDefault,
} from "is-glob";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hasClosedBracket` | `(str: string, open_idx: usize): bool` | Checks predicate `hasClosedBracket` and returns `bool`. |
| `hasClosedBrace` | `(str: string, open_idx: usize): bool` | Checks predicate `hasClosedBrace` and returns `bool`. |
| `hasUnescapedWildcard` | `(str: string): bool` | Checks predicate `hasUnescapedWildcard` and returns `bool`. |
| `isGlob` | `(str: string, strict: bool): bool` | Checks predicate `isGlob` and returns `bool`. |
| `isGlobDefault` | `(str: string): bool` | Checks predicate `isGlobDefault` and returns `bool`. |

## Minimal Example

```cp
import { isGlob } from "is-glob";

pub fn main() {
    let result = isGlob("example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
