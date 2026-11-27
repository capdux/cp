[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# listr2

Zero-heap CLI task list state machine with pending/running/completed/skipped/failed lifecycle, retries, and ANSI rendering (npm listr2 equivalent, reuses picocolors)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:listr2` ([https://github.com/listr2/listr2](https://github.com/listr2/listr2))

| Metadata | Value |
| --- | --- |
| Package | `listr2` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add listr2
```

```cp
import { ListrOptions, TaskNode, ListrRunner, lstrPush } from "listr2";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ListrOptions` | `class` | Fields: `concurrent: bool, exit_on_error: bool, use_color: bool` · Methods: `defaults` |
| `TaskNode` | `class` | Fields: `title: string, state: u8, retries_left: u8, attempts: u8, skip_reason: string, err_msg: string` |
| `ListrRunner` | `class` | Fields: `opts: ListrOptions, tasks: [32]TaskNode, task_count: usize, halted: bool` · Methods: `new, addTask, startTask, completeTask, skipTask, failTask, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lstrPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `lstrPush` and returns `usize`. |

## Minimal Example

```cp
import { lstrPush } from "listr2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = lstrPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
