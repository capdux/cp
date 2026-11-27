[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# relative-path

Portable platform-independent relative path manipulation, normalization, joining, and resolution (zero-heap pure CP)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`relative-path`](https://github.com/udoprog/relative-path)（`MIT OR Apache-2.0`），以 Clean-room MIT 纯 CP 零堆方式实现，包名 `relative-path`。

| Metadata | Value |
| --- | --- |
| Package | `relative-path` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add relative-path
```

```cp
import {
    RelPath,
    rpIsSep,
    normalizeRelPath,
    joinRelPath,
    relativeTo,
} from "relative-path";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RelPath` | `class` | Fields: `buf: [256]u8, length: usize` · Methods: `empty, fromStr, len, isEmpty, equals, copyTo, +10 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rpIsSep` | `(b: u8): bool` | Invokes `rpIsSep` and returns `bool`. |
| `normalizeRelPath` | `(path: string): RelPath` | Invokes `normalizeRelPath` and returns `RelPath`. |
| `joinRelPath` | `(base: string, rel: string): RelPath` | Invokes `joinRelPath` and returns `RelPath`. |
| `relativeTo` | `(from_path: string, to_path: string): RelPath` | Invokes `relativeTo` and returns `RelPath`. |

## Minimal Example

```cp
import { normalizeRelPath } from "relative-path";

pub fn main() {
    let result = normalizeRelPath("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
