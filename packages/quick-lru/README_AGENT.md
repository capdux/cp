# quick-lru — AI Agent Quick Ref

Zero-heap two-generation QuickLRU cache (QuickLruI64) with O(1) rotation eviction (clean-room port of npm quick-lru) (`v1.0.0`, `MIT`)

## Import

```cp
import { QlruSlot, QuickLruI64, qlruStrEq } from "quick-lru";
```

## Core Types & Constants

```cp
class QlruSlot { occupied: bool, key: string, val: i64, hash: u64 }
class QuickLruI64 { cur: [32]QlruSlot, old: [32]QlruSlot, cur_len: usize, old_len: usize, /* +1 fields */ } // methods: new(...) +12
```

## API Signatures

```cp
fn qlruStrEq(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { qlruStrEq } from "quick-lru";

pub fn main() {
    let result = qlruStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"quick-lru"`; install via `cap add quick-lru`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
