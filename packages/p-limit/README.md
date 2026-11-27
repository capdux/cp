[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# p-limit

Zero-allocation concurrency limiter and FIFO task slot scheduler (p-limit parity) powered by yocto-queue in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`p-limit`](https://github.com/sindresorhus/p-limit)（`MIT`）（包名 `p-limit`）。

| Metadata | Value |
| --- | --- |
| Package | `p-limit` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add p-limit
```

```cp
import { SubmitResult, CompleteResult, PLimit, plClampConcurrency } from "p-limit";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SubmitResult` | `class` | Fields: `started_immediately: bool, queued: bool, rejected: bool` |
| `CompleteResult` | `class` | Fields: `has_next: bool, next_task_id: u64` |
| `PLimit` | `class` | Fields: `limit: usize, active: usize, queue: YoctoQueueU64` · Methods: `new, activeCount, pendingCount, queueCapacity, concurrency, setConcurrency, +7 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `plClampConcurrency` | `(c: usize): usize` | Invokes `plClampConcurrency` and returns `usize`. |

## Minimal Example

```cp
import { plClampConcurrency } from "p-limit";

pub fn main() {
    let result = plClampConcurrency(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
