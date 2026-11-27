[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# p-settle

Zero-allocation bounded-concurrency Promise.allSettled task planner (npm p-settle v5 parity) powered by p-limit in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`p-settle`](https://github.com/sindresorhus/p-settle)（`MIT`）（包名 `p-settle`）。

| Metadata | Value |
| --- | --- |
| Package | `p-settle` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add p-settle
```

```cp
import { SettleStatus, SettledEntry, PSettlePlanner, psetFindSlot } from "p-settle";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SettleStatus` | `Pending, Fulfilled, Rejected` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SettledEntry` | `class` | Fields: `id: u64, status: SettleStatus, value: i64, err_code: i32` |
| `PSettlePlanner` | `class` | Fields: `limiter: PLimit, total_count: usize, fulfilled_count: usize, rejected_count: usize, ids: [32]u64, running: [32]bool, +3 more` · Methods: `new, addTask, dispatchNext, reportFulfilled, reportRejected, isAllSettled, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `psetFindSlot` | `(ids: [32]u64, count: usize, id: u64): usize?` | Invokes `psetFindSlot` and returns `usize?`. |

## Minimal Example

```cp
import { psetFindSlot } from "p-settle";

pub fn main() {
    let result = psetFindSlot(/* ids: [32]u64 */, 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
