[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# strum

Zero-heap enum variant string registry, case-insensitive parser, discriminant lookup, and iteration (clean-room port of Rust strum)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:strum` ([https://github.com/Peternator7/strum](https://github.com/Peternator7/strum))（原库名 `strum` → CP 包名 `strum`）

| Metadata | Value |
| --- | --- |
| Package | `strum` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add strum
```

```cp
import { EnumVariant, EnumRegistry, stmStrEq } from "strum";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EnumVariant` | `class` | Fields: `id: i32, name: string, alias: string, message: string` |
| `EnumRegistry` | `class` | Fields: `variants: [32]EnumVariant, count: usize` · Methods: `new, variantCount, register, fromStr, fromStrIgnoreCase, asStr, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `stmStrEq` | `(a: string, b: string): bool` | Invokes `stmStrEq` and returns `bool`. |

## Minimal Example

```cp
import { stmStrEq } from "strum";

pub fn main() {
    let result = stmStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
