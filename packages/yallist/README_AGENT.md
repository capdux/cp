# yallist — AI Agent Quick Ref

Zero-allocation O(1) doubly linked list backed by a fixed 64-slot freelist pool (npm yallist v5 parity) in pure CP (`v1.0.0`, `ISC`)

## Import

```cp
import { YallistU64, yalValidHandle } from "yallist";
```

## Core Types & Constants

```cp
class YallistU64 { values: [64]u64, prev: [64]i32, next: [64]i32, alive: [64]bool, /* +5 fields */ } // methods: new(...) +15
```

## API Signatures

```cp
fn yalValidHandle(handle: i32): bool;
```

## Minimal Usage

```cp
import { yalValidHandle } from "yallist";

pub fn main() {
    let result = yalValidHandle(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"yallist"`; install via `cap add yallist`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add yallist
cpc check src/main.cp
```
