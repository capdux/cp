# p-all — AI Agent Quick Ref

Zero-allocation bounded-concurrency batch task planner (npm p-all v5 parity) powered by p-limit in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { PAllPlanner, pallFindSlot } from "p-all";
```

## Core Types & Constants

```cp
class PAllPlanner { limiter: PLimit, stop_on_error: bool, failed: bool, first_err: i32, /* +7 fields */ } // methods: new(...) +15
```

## API Signatures

```cp
fn pallFindSlot(ids: [32]u64, count: usize, id: u64): usize?;
```

## Minimal Usage

```cp
import { pallFindSlot } from "p-all";

pub fn main() {
    let result = pallFindSlot(/* ids: [32]u64 */, 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"p-all"`; install via `cap add p-all`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add p-all
cpc check src/main.cp
```
