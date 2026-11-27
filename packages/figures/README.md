[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# figures

Zero-allocation Unicode terminal symbols with automatic ASCII/Windows fallbacks composing is-unicode-supported in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:figures` ([https://github.com/sindresorhus/figures](https://github.com/sindresorhus/figures), `MIT`)

| Metadata | Value |
| --- | --- |
| Package | `figures` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add figures
```

```cp
import {
    FigureKind,
    figStartsAt,
    figWriteStr,
    figureFor,
    figureForEnv,
    replaceSymbols,
} from "figures";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `FigureKind` | `Tick, Cross, Star, Square, Circle, Info, Warning, Pointer ...` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `figStartsAt` | `(input: string, pos: usize, needle: string): bool` | Invokes `figStartsAt` and returns `bool`. |
| `figWriteStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `figWriteStr` and returns `usize`. |
| `figureFor` | `(kind: FigureKind, unicode_ok: bool): string` | Invokes `figureFor` and returns `string`. |
| `figureForEnv` | `(kind: FigureKind, env: EnvInfo): string` | Invokes `figureForEnv` and returns `string`. |
| `replaceSymbols` | `(input: string, unicode_ok: bool, out: []u8): usize` | Invokes `replaceSymbols` and returns `usize`. |

## Minimal Example

```cp
import { figStartsAt } from "figures";

pub fn main() {
    let result = figStartsAt("example", 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
