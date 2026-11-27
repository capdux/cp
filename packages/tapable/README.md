[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tapable

Zero-allocation plugin hook engine (SyncHook, SyncBailHook, SyncWaterfallHook, SyncLoopHook) with stage ordering and before insertion (clean-room MIT implementation of npm tapable v2, #242 / 195M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:tapable` ([https://github.com/webpack/tapable](https://github.com/webpack/tapable)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `tapable` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tapable
```

```cp
import {
    TapEntry,
    SyncHook,
    SyncBailHook,
    SyncWaterfallHook,
    SyncLoopHook,
    tapEmptyEntry,
    tapStrEq,
    tapInsertSorted,
} from "tapable";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TapEntry` | `class` | Fields: `name: string, stage: i32, handler_id: u32, enabled: bool` |
| `SyncHook` | `class` | Fields: `entries: [32]TapEntry, count: usize` · Methods: `init, tap, tapStage, tapBefore, setEnabled, isUsed, +3 more` |
| `SyncBailHook` | `class` | Fields: `entries: [32]TapEntry, count: usize` · Methods: `init, tap, tapStage, tapBefore, setEnabled, isUsed, +3 more` |
| `SyncWaterfallHook` | `class` | Fields: `entries: [32]TapEntry, count: usize` · Methods: `init, tap, tapStage, tapBefore, setEnabled, isUsed, +3 more` |
| `SyncLoopHook` | `class` | Fields: `entries: [32]TapEntry, count: usize` · Methods: `init, tap, tapStage, tapBefore, setEnabled, isUsed, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `tapEmptyEntry` | `(): TapEntry` | Invokes `tapEmptyEntry` and returns `TapEntry`. |
| `tapStrEq` | `(a: string, b: string): bool` | Invokes `tapStrEq` and returns `bool`. |
| `tapInsertSorted` | `(entries: []TapEntry, count: usize, entry: TapEntry): usize` | Invokes `tapInsertSorted` and returns `usize`. |
| `tapInsertBefore` | `(entries: []TapEntry, count: usize, name: string, before_name: string, handler_id: u32): usize` | Invokes `tapInsertBefore` and returns `usize`. |
| `tapSetEnabled` | `(entries: []TapEntry, count: usize, name: string, enabled: bool): bool` | Invokes `tapSetEnabled` and returns `bool`. |
| `tapCollectOrdered` | `(entries: &[]TapEntry, count: usize, out: []u32): usize` | Invokes `tapCollectOrdered` and returns `usize`. |

## Minimal Example

```cp
import { tapStrEq } from "tapable";

pub fn main() {
    let result = tapStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
