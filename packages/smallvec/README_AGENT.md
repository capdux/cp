# smallvec — AI Agent Quick Ref

Zero-heap inline stack vector with push/pop/insert/remove/swapRemove/dedup (clean-room CP implementation of crates.io smallvec) (`v1.0.0`, `MIT`)

## Import

```cp
import { SmallPopU64, SmallVecU64, SmallVecU8 } from "smallvec";
```

## Core Types & Constants

```cp
class SmallPopU64 { ok: bool, value: u64 }
class SmallVecU64 { data: [64]u64, used: usize } // methods: new(...) +17
class SmallVecU8 { data: [256]u8, used: usize } // methods: new(...) +7
```

## Minimal Usage

```cp
import { SmallPopU64 } from "smallvec";

pub fn main() {
    // Refer to src/index.d.cp for SmallPopU64 fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"smallvec"`; install via `cap add smallvec`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add smallvec
cpc check src/main.cp
```
