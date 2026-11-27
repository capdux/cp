[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ordered-multimap

Zero-heap insertion-ordered multimap supporting multiple values per key

| Metadata | Value |
| --- | --- |
| Package | `ordered-multimap` |
| Version | `0.7.3` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ordered-multimap
```

```cp
import { OrderedMultiMap, ommEmptyStrings, ommStrEq } from "ordered-multimap";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OrderedMultiMap` | `class` | Fields: `keys: [OMM_CAP]string, vals: [OMM_CAP]string, count: usize` · Methods: `new, append, insert, getFirst, getAll, removeAll, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ommEmptyStrings` | `(): [OMM_CAP]string` | Invokes `ommEmptyStrings` and returns `[OMM_CAP]string`. |
| `ommStrEq` | `(a: string, b: string): bool` | Invokes `ommStrEq` and returns `bool`. |

## Minimal Example

```cp
import { ommStrEq } from "ordered-multimap";

pub fn main() {
    let result = ommStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
