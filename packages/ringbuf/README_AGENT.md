# ringbuf — AI Agent Quick Ref

Zero-heap O(1) circular ring buffers (RingBufU8 & RingBufI64) with slice batching and overwrite mode (clean-room port of Rust ringbuf) (`v1.0.0`, `MIT`)

## Import

```cp
import { RingBufU8, RingBufI64 } from "ringbuf";
```

## Core Types & Constants

```cp
class RingBufU8 { buf: [256]u8, head: usize, tail: usize, count: usize } // methods: new(...) +12
class RingBufI64 { buf: [64]i64, head: usize, tail: usize, count: usize } // methods: new(...) +9
```

## Minimal Usage

```cp
import { RingBufU8 } from "ringbuf";

pub fn main() {
    // Refer to src/index.d.cp for RingBufU8 fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ringbuf"`; install via `cap add ringbuf`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add ringbuf
cpc check src/main.cp
```
