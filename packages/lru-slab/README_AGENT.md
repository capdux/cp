# lru-slab — AI Agent Quick Ref

Zero-heap pre-allocated O(1) intrusive LRU slab cache matching crates.io #415 lru-slab (358M+ downloads) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { GetResult, EvictResult, SlabEntry, LruSlab } from "lru-slab";
```

## Core Types & Constants

```cp
class GetResult { ok: bool, val: u64 }
class EvictResult { idx: i32, key: u64, val: u64 }
class SlabEntry { occupied: bool, key: u64, val: u64, prev: i32, /* +1 fields */ }
class LruSlab { head: i32, tail: i32, free_head: i32, count: usize, /* +1 fields */ } // methods: new(...) +13
```

## Minimal Usage

```cp
import { GetResult } from "lru-slab";

pub fn main() {
    // Refer to src/index.d.cp for GetResult fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lru-slab"`; install via `cap add lru-slab`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add lru-slab
cpc check src/main.cp
```
