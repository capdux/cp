# thin-vec — AI Agent Quick Ref

Zero-heap compact ThinVec<i64> with push/pop/insert/remove/swapRemove/dedup/truncate and packed metadata header (crates.io thin-vec equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import { ThinVecI64 } from "thin-vec";
```

## Core Types & Constants

```cp
class ThinVecI64 { len_val: usize, data: [64]i64 } // methods: new(...) +16
```

## Minimal Usage

```cp
import { ThinVecI64 } from "thin-vec";

pub fn main() {
    // Refer to src/index.d.cp for ThinVecI64 fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"thin-vec"`; install via `cap add thin-vec`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add thin-vec
cpc check src/main.cp
```
