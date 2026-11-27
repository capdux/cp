[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# on-finished

Zero-heap HTTP message/stream completion tracker with once-only finish/error/close lifecycle dispatch (npm on-finished equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:on-finished` ([https://github.com/jshttp/on-finished](https://github.com/jshttp/on-finished))

| Metadata | Value |
| --- | --- |
| Package | `on-finished` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add on-finished
```

```cp
import { ListenerSlot, StreamLifecycle, isFinished } from "on-finished";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ListenerSlot` | `class` | Fields: `id: u32, fired: bool, reason: u8, err_code: i32` |
| `StreamLifecycle` | `class` | Fields: `readable_ended: bool, writable_finished: bool, socket_closed: bool, errored: bool, err_code: i32, finish_reason: u8, +3 more` · Methods: `new, isFinished, isReadableFinished, isWritableFinished, onFinished, dispatchAll, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isFinished` | `(lc: StreamLifecycle): bool` | Checks predicate `isFinished` and returns `bool`. |

## Minimal Example

```cp
import { isFinished } from "on-finished";

pub fn main() {
    let result = isFinished(/* lc: StreamLifecycle */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
