# yocto-queue — AI Agent Quick Ref

Tiny O(1) FIFO queue data structure (clean-room zero-heap CP implementation of npm yocto-queue) (`v1.0.0`, `MIT`)

## Import

```cp
import { PopU64Result, PopI64Result, YoctoQueueU64, YoctoQueueI64 } from "yocto-queue";
```

## Core Types & Constants

```cp
class PopU64Result { ok: bool, value: u64 }
class PopI64Result { ok: bool, value: i64 }
class YoctoQueueU64 { buf: [256]u64, head: usize, tail: usize, count: usize } // methods: new(...) +10
class YoctoQueueI64 { buf: [256]i64, head: usize, tail: usize, count: usize } // methods: new(...) +7
```

## Minimal Usage

```cp
import { PopU64Result } from "yocto-queue";

pub fn main() {
    // Refer to src/index.d.cp for PopU64Result fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"yocto-queue"`; install via `cap add yocto-queue`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add yocto-queue
cpc check src/main.cp
```
