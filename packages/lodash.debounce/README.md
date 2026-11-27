[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# lodash.debounce

Zero-allocation deterministic clock-driven Debouncer and Throttler state machines with leading/trailing/maxWait semantics in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lodash.debounce` ([https://github.com/lodash/lodash](https://github.com/lodash/lodash))（原库名 `lodash.debounce` → CP 包名 `lodash.debounce`）

| Metadata | Value |
| --- | --- |
| Package | `lodash.debounce` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add lodash.debounce
```

```cp
import { Debouncer, Throttler, elapsedSince } from "lodash.debounce";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Debouncer` | `class` | Fields: `wait_ms: u64, leading: bool, trailing: bool, max_wait_ms: u64, active: bool, pending: bool, +3 more` · Methods: `init, trigger, poll, flush, cancel, isPending, +2 more` |
| `Throttler` | `class` | Fields: `interval_ms: u64, leading: bool, trailing: bool, has_window: bool, window_start_ms: u64, pending: bool, +1 more` · Methods: `init, trigger, poll, cancel, isPending, execCount` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `elapsedSince` | `(now_ms: u64, past_ms: u64): u64` | Invokes `elapsedSince` and returns `u64`. |

## Minimal Example

```cp
import { elapsedSince } from "lodash.debounce";

pub fn main() {
    let result = elapsedSince(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
