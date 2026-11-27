# bimap — AI Agent Quick Ref

Zero-heap bidirectional 1-to-1 map (left <-> right) in pure CP, modeled after Rust bimap (`v1.0.0`, `MIT`)

## Import

```cp
import { BiPair, BiMap } from "bimap";
```

## Core Types & Constants

```cp
class BiPair { left: u64, right: u64 }
class BiMap { pairs: [128]BiPair, count: usize } // methods: init(...) +14
```

## Minimal Usage

```cp
import { BiPair } from "bimap";

pub fn main() {
    // Refer to src/index.d.cp for BiPair fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bimap"`; install via `cap add bimap`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add bimap
cpc check src/main.cp
```
