# p-settle — AI Agent Quick Ref

Zero-allocation bounded-concurrency Promise.allSettled task planner (npm p-settle v5 parity) powered by p-limit in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { SettleStatus, SettledEntry, PSettlePlanner, psetFindSlot } from "p-settle";
```

## Core Types & Constants

```cp
enum SettleStatus { Pending, Fulfilled, Rejected }
class SettledEntry { id: u64, status: SettleStatus, value: i64, err_code: i32 }
class PSettlePlanner { limiter: PLimit, total_count: usize, fulfilled_count: usize, rejected_count: usize, /* +5 fields */ } // methods: new(...) +14
```

## API Signatures

```cp
fn psetFindSlot(ids: [32]u64, count: usize, id: u64): usize?;
```

## Minimal Usage

```cp
import { psetFindSlot } from "p-settle";

pub fn main() {
    let result = psetFindSlot(/* ids: [32]u64 */, 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"p-settle"`; install via `cap add p-settle`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
