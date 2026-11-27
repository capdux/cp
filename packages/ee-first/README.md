[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ee-first

Zero-heap pure CP multi-emitter first-event race listener with automatic cleanup (clean-room MIT implementation of npm ee-first v1)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ee-first` ([https://github.com/jonathanong/ee-first](https://github.com/jonathanong/ee-first))

| Metadata | Value |
| --- | --- |
| Package | `ee-first` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ee-first
```

```cp
import { EventWatch, FirstResult, FirstRace, eefStrEq } from "ee-first";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EventWatch` | `class` | Fields: `emitter_id: u32, event_name: string, active: bool` |
| `FirstResult` | `class` | Fields: `triggered: bool, emitter_id: u32, event_name: string, payload_code: i32, cancelled_count: usize` · Methods: `empty` |
| `FirstRace` | `class` | Fields: `watchers: [16]EventWatch, count: usize, settled: bool, result: FirstResult` · Methods: `new, add, emit, cancel, activeCount, isSettled, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `eefStrEq` | `(a: string, b: string): bool` | Invokes `eefStrEq` and returns `bool`. |

## Minimal Example

```cp
import { eefStrEq } from "ee-first";

pub fn main() {
    let result = eefStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
