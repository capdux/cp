[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# shebang-regex

Zero-allocation shebang (#!) line matcher and stripper for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shebang-regex` ([https://github.com/sindresorhus/shebang-regex](https://github.com/sindresorhus/shebang-regex))

| Metadata | Value |
| --- | --- |
| Package | `shebang-regex` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add shebang-regex
```

```cp
import { ShebangMatch, hasShebang, matchShebang, stripShebang } from "shebang-regex";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ShebangMatch` | `class` | Fields: `matched: bool, line_len: usize, cmd_start: usize, cmd_len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hasShebang` | `(input: string): bool` | Checks predicate `hasShebang` and returns `bool`. |
| `matchShebang` | `(input: string): ShebangMatch` | Invokes `matchShebang` and returns `ShebangMatch`. |
| `stripShebang` | `(input: string): usize` | Invokes `stripShebang` and returns `usize`. |

## Minimal Example

```cp
import { hasShebang } from "shebang-regex";

pub fn main() {
    let result = hasShebang("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
