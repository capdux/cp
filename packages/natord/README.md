[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# natord

Zero-heap arbitrary-precision human natural sort comparator, case-insensitive comparator, and in-place sorter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:natord` ([https://github.com/lifthrasiir/rust-natord](https://github.com/lifthrasiir/rust-natord))（原库名 `natord` → CP 包名 `natord`）

| Metadata | Value |
| --- | --- |
| Package | `natord` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add natord
```

```cp
import {
    compareImpl,
    compareNatural,
    compareNaturalIgnoreCase,
    isSortedNatural,
    sortNatural,
} from "natord";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `compareImpl` | `(a: string, b: string, ignore_case: bool): i32` | Invokes `compareImpl` and returns `i32`. |
| `compareNatural` | `(a: string, b: string): i32` | Invokes `compareNatural` and returns `i32`. |
| `compareNaturalIgnoreCase` | `(a: string, b: string): i32` | Invokes `compareNaturalIgnoreCase` and returns `i32`. |
| `isSortedNatural` | `(items: &[]string): bool` | Checks predicate `isSortedNatural` and returns `bool`. |
| `sortNatural` | `(items: []string)` | Executes `sortNatural` in-place. |

## Minimal Example

```cp
import { isSortedNatural } from "natord";

pub fn main() {
    let result = isSortedNatural(/* items: &[]string */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
