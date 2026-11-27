# tinyvec — AI Agent Quick Ref

Zero-heap 100%-safe inline stack vector (TinyVecI64 & TinyVecU32) with insert, swapRemove, dedup, reverse, and binarySearch (clean-room port of Rust tinyvec) (`v1.0.0`, `MIT`)

## Import

```cp
import { TinyVecI64, TinyVecU32 } from "tinyvec";
```

## Core Types & Constants

```cp
class TinyVecI64 { data: [64]i64, used: usize } // methods: new(...) +19
class TinyVecU32 { data: [64]u32, used: usize } // methods: new(...) +7
```

## Minimal Usage

```cp
import { TinyVecI64 } from "tinyvec";

pub fn main() {
    // Refer to src/index.d.cp for TinyVecI64 fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tinyvec"`; install via `cap add tinyvec`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add tinyvec
cpc check src/main.cp
```
