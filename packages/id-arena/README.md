[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# id-arena

Zero-allocation typed handle arena allocator with arena_id cross-arena misuse protection in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:id-arena` ([https://github.com/fitzgen/id-arena](https://github.com/fitzgen/id-arena), `MIT OR Apache-2.0`)

| Metadata | Value |
| --- | --- |
| Package | `id-arena` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add id-arena
```

```cp
import { ArenaId, IdArenaI64, IdArenaU64, idaIsValid } from "id-arena";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ArenaId` | `class` | Fields: `index: usize, arena_id: u32` |
| `IdArenaI64` | `class` | Fields: `arena_id: u32, items: [128]i64, count: usize` · Methods: `new, alloc, get, set, len, isEmpty, +3 more` |
| `IdArenaU64` | `class` | Fields: `arena_id: u32, items: [128]u64, count: usize` · Methods: `new, alloc, get, set, len, isEmpty, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `idaIsValid` | `(id: ArenaId, expected_arena_id: u32, count: usize): bool` | Invokes `idaIsValid` and returns `bool`. |

## Minimal Example

```cp
import { idaIsValid } from "id-arena";

pub fn main() {
    let result = idaIsValid(/* id: ArenaId */, 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
