# priority-queue — AI Agent Quick Ref

Zero-allocation Indexed Binary Min-Heap with O(log N) dynamic changePriority and arbitrary remove (Rust priority-queue / heap-js parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { PqItem, IndexedMinHeap } from "priority-queue";
```

## Core Types & Constants

```cp
class PqItem { id: u16, priority: i64, valid: bool }
class IndexedMinHeap { heap_ids: [MAX_ITEMS]u16, priorities: [MAX_ITEMS]i64, pos: [MAX_ITEMS]i16, size: usize } // methods: init(...) +14
```

## Minimal Usage

```cp
import { PqItem } from "priority-queue";

pub fn main() {
    // Refer to src/index.d.cp for PqItem fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"priority-queue"`; install via `cap add priority-queue`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add priority-queue
cpc check src/main.cp
```
