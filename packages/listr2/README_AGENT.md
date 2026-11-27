# listr2 — AI Agent Quick Ref

Zero-heap CLI task list state machine with pending/running/completed/skipped/failed lifecycle, retries, and ANSI rendering (npm listr2 equivalent, reuses picocolors) (`v1.0.0`, `MIT`)

## Import

```cp
import { ListrOptions, TaskNode, ListrRunner, lstrPush } from "listr2";
```

## Core Types & Constants

```cp
class ListrOptions { concurrent: bool, exit_on_error: bool, use_color: bool } // methods: defaults(...)
class TaskNode { title: string, state: u8, retries_left: u8, attempts: u8, /* +2 fields */ }
class ListrRunner { opts: ListrOptions, tasks: [32]TaskNode, task_count: usize, halted: bool } // methods: new(...) +7
```

## API Signatures

```cp
fn lstrPush(out: []u8, pos: usize, s: string): usize;
```

## Minimal Usage

```cp
import { lstrPush } from "listr2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = lstrPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"listr2"`; install via `cap add listr2`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
