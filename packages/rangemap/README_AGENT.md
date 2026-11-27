# rangemap — AI Agent Quick Ref

Zero-allocation sorted disjoint half-open interval set [start, end) with automatic coalescing, subtraction/splitting, binary-search lookup, and span calculation (Rust rangemap / npm merge-ranges parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { Range, RangeSet } from "rangemap";
```

## Core Types & Constants

```cp
class Range { start: i64, end: i64 }
class RangeSet { ranges: [MAX_RANGES]Range, count: usize } // methods: init(...) +11
```

## Minimal Usage

```cp
import { Range } from "rangemap";

pub fn main() {
    // Refer to src/index.d.cp for Range fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rangemap"`; install via `cap add rangemap`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add rangemap
cpc check src/main.cp
```
