[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# multimap

Zero-allocation insertion-ordered one-to-many string multimap (capacity 128) for HTTP headers, URL query params, tag indexes, and event routing (Rust multimap parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:multimap` ([https://github.com/havarnov/multimap](https://github.com/havarnov/multimap))

| Metadata | Value |
| --- | --- |
| Package | `multimap` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add multimap
```

```cp
import { MultiEntry, MultiMapStr, strEq } from "multimap";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MultiEntry` | `class` | Fields: `key: string, value: string` |
| `MultiMapStr` | `class` | Fields: `entries: [MULTIMAP_CAP]MultiEntry, count: usize` · Methods: `init, totalValues, isEmpty, isFull, clear, insert, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `strEq` | `(a: string, b: string): bool` | Invokes `strEq` and returns `bool`. |

## Minimal Example

```cp
import { strEq } from "multimap";

pub fn main() {
    let result = strEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
