[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mitt

Zero-allocation tiny functional event emitter with wildcard '*' support in pure CP (npm mitt v3 parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mitt` ([https://github.com/developit/mitt](https://github.com/developit/mitt))

| Metadata | Value |
| --- | --- |
| Package | `mitt` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mitt
```

```cp
import {
    MittListener,
    MittDispatch,
    Emitter,
    mitStrEq,
    mitIsWildcard,
    mitt,
} from "mitt";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MittListener` | `class` | Fields: `active: bool, event_type: string, handler_id: u32, is_wildcard: bool` · Methods: `empty` |
| `MittDispatch` | `class` | Fields: `handler_id: u32, payload: i64, is_wildcard: bool` · Methods: `empty` |
| `Emitter` | `class` | Fields: `listeners: [32]MittListener, count: usize, last_dispatched: [32]MittDispatch, last_dispatch_count: usize, emit_count: usize` · Methods: `new, on, off, emit, listenerCount, totalListeners, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mitStrEq` | `(a: string, b: string): bool` | Invokes `mitStrEq` and returns `bool`. |
| `mitIsWildcard` | `(event_type: string): bool` | Invokes `mitIsWildcard` and returns `bool`. |
| `mitt` | `(): Emitter` | Invokes `mitt` and returns `Emitter`. |

## Minimal Example

```cp
import { mitStrEq } from "mitt";

pub fn main() {
    let result = mitStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
