# slab — AI Agent Quick Ref

Zero-heap pre-allocated O(1) slab allocator with intrusive free-list (clean-room port of Rust slab) (`v1.0.0`, `MIT`)

## Import

```cp
import { SlabEntryI64, SlabI64 } from "slab";
```

## Core Types & Constants

```cp
class SlabEntryI64 { occupied: bool, val: i64, next_free: usize }
class SlabI64 { entries: [128]SlabEntryI64, count: usize, next_free: usize, watermark: usize } // methods: new(...) +12
```

## Minimal Usage

```cp
import { SlabEntryI64 } from "slab";

pub fn main() {
    // Refer to src/index.d.cp for SlabEntryI64 fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"slab"`; install via `cap add slab`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add slab
cpc check src/main.cp
```
