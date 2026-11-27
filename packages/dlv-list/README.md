[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# dlv-list

Zero-heap Vec-backed doubly linked list with stable u32 handles and O(1) slot reuse

| Metadata | Value |
| --- | --- |
| Package | `dlv-list` |
| Version | `0.5.2` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add dlv-list
```

```cp
import {
    DlvListI64,
    dlvlZeroI64,
    dlvlNoneU32,
    dlvlFalseBool,
    dlvlAllocSlot,
} from "dlv-list";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DlvListI64` | `class` | Fields: `values: [DLV_CAP]i64, prev: [DLV_CAP]u32, next: [DLV_CAP]u32, occupied: [DLV_CAP]bool, head: u32, tail: u32, +3 more` · Methods: `new, pushFront, pushBack, get, remove, popFront, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `dlvlZeroI64` | `(): [DLV_CAP]i64` | Invokes `dlvlZeroI64` and returns `[DLV_CAP]i64`. |
| `dlvlNoneU32` | `(): [DLV_CAP]u32` | Invokes `dlvlNoneU32` and returns `[DLV_CAP]u32`. |
| `dlvlFalseBool` | `(): [DLV_CAP]bool` | Invokes `dlvlFalseBool` and returns `[DLV_CAP]bool`. |
| `dlvlAllocSlot` | `(list: &mut DlvListI64): u32` | Invokes `dlvlAllocSlot` and returns `u32`. |

## Minimal Example

```cp
import { dlvlZeroI64 } from "dlv-list";

pub fn main() {
    let result = dlvlZeroI64();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
