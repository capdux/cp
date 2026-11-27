[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# p-all

Zero-allocation bounded-concurrency batch task planner (npm p-all v5 parity) powered by p-limit in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`p-all`](https://github.com/sindresorhus/p-all)（`MIT`）（包名 `p-all`）。

| Metadata | Value |
| --- | --- |
| Package | `p-all` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add p-all
```

```cp
import { PAllPlanner, pallFindSlot } from "p-all";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PAllPlanner` | `class` | Fields: `limiter: PLimit, stop_on_error: bool, failed: bool, first_err: i32, total_count: usize, completed_count: usize, +5 more` · Methods: `new, addTask, dispatchNext, reportSuccess, reportError, isDone, +10 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pallFindSlot` | `(ids: [32]u64, count: usize, id: u64): usize?` | Invokes `pallFindSlot` and returns `usize?`. |

## Minimal Example

```cp
import { pallFindSlot } from "p-all";

pub fn main() {
    let result = pallFindSlot(/* ids: [32]u64 */, 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
