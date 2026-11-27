# crossbeam-queue — AI Agent Quick Ref

Zero-heap bounded ring queue (RingQueueU64) and wrap-around byte ring buffer (ByteRing) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { RingQueueU64, ByteRing } from "crossbeam-queue";
```

## Core Types & Constants

```cp
class RingQueueU64 { head: usize, tail: usize, count: usize, capacity: usize } // methods: new(...) +9
class ByteRing { head: usize, tail: usize, count: usize, capacity: usize } // methods: new(...) +7
```

## Minimal Usage

```cp
import { RingQueueU64 } from "crossbeam-queue";

pub fn main() {
    // Refer to src/index.d.cp for RingQueueU64 fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"crossbeam-queue"`; install via `cap add crossbeam-queue`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add crossbeam-queue
cpc check src/main.cp
```
