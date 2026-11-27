# sharded-slab — AI Agent Quick Ref

Zero-allocation O(1) generational slab allocator and ABA-safe handle pool (Rust slab + generational-arena parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { SlabKey, SlabI64, SlabU64, nextGeneration } from "sharded-slab";
```

## Core Types & Constants

```cp
class SlabKey { index: u32, generation: u32 } // methods: equals(...)
class SlabI64 { slots: [SLAB_CAPACITY]i64, occupied: [SLAB_CAPACITY]bool, generations: [SLAB_CAPACITY]u32, next_free: [SLAB_CAPACITY]i32, /* +4 fields */ } // methods: init(...) +9
class SlabU64 { slots: [SLAB_CAPACITY]u64, occupied: [SLAB_CAPACITY]bool, generations: [SLAB_CAPACITY]u32, next_free: [SLAB_CAPACITY]i32, /* +4 fields */ } // methods: init(...) +9
```

## API Signatures

```cp
fn nextGeneration(gen: u32): u32;
```

## Minimal Usage

```cp
import { nextGeneration } from "sharded-slab";

pub fn main() {
    let result = nextGeneration(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sharded-slab"`; install via `cap add sharded-slab`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
