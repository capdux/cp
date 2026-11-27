[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# litemap

Zero-allocation compact sorted flat-array map with O(log N) binary search and deterministic ordered iteration (Rust ICU4X litemap parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:litemap` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))

| Metadata | Value |
| --- | --- |
| Package | `litemap` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add litemap
```

```cp
import {
    SearchResult,
    StrEntry,
    StrInsertResult,
    U64Entry,
    U64InsertResult,
    LiteMapStr,
    LiteMapU64,
    cmpStr,
} from "litemap";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SearchResult` | `class` | Fields: `found: bool, index: usize` |
| `StrEntry` | `class` | Fields: `key: string, value: string` |
| `StrInsertResult` | `class` | Fields: `ok: bool, replaced: bool, prev: string` |
| `U64Entry` | `class` | Fields: `key: u64, value: i64` |
| `U64InsertResult` | `class` | Fields: `ok: bool, replaced: bool, prev: i64` |
| `LiteMapStr` | `class` | Fields: `entries: [LITEMAP_CAP]StrEntry, count: usize` · Methods: `init, len, isEmpty, isFull, clear, binarySearch, +5 more` |
| `LiteMapU64` | `class` | Fields: `entries: [LITEMAP_CAP]U64Entry, count: usize` · Methods: `init, len, isEmpty, isFull, clear, binarySearch, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cmpStr` | `(a: string, b: string): i8` | Invokes `cmpStr` and returns `i8`. |

## Minimal Example

```cp
import { cmpStr } from "litemap";

pub fn main() {
    let result = cmpStr("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
