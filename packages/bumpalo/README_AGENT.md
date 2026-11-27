# bumpalo — AI Agent Quick Ref

Zero-allocation alignment-aware bump arena allocator (BumpArena, BumpSlice) with checkpoint/rewind and overflow protection (Rust bumpalo v3 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BumpSlice,
    BumpArena,
    bmpIsValidAlign,
    bmpAlignUp,
    bmpHandleValid,
} from "bumpalo";
```

## Core Types & Constants

```cp
class BumpSlice { offset: usize, len: usize } // methods: empty(...) +2
class BumpArena { buf: [4096]u8, cursor: usize, alloc_count: usize } // methods: new(...) +14
```

## API Signatures

```cp
fn bmpIsValidAlign(alignment: usize): bool;
fn bmpAlignUp(offset: usize, alignment: usize): usize?;
fn bmpHandleValid(handle: BumpSlice, cursor: usize): bool;
```

## Minimal Usage

```cp
import { bmpIsValidAlign } from "bumpalo";

pub fn main() {
    let result = bmpIsValidAlign(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bumpalo"`; install via `cap add bumpalo`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
