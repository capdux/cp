# lodash.debounce — AI Agent Quick Ref

Zero-allocation deterministic clock-driven Debouncer and Throttler state machines with leading/trailing/maxWait semantics in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { Debouncer, Throttler, elapsedSince } from "lodash.debounce";
```

## Core Types & Constants

```cp
class Debouncer { wait_ms: u64, leading: bool, trailing: bool, max_wait_ms: u64, /* +5 fields */ } // methods: init(...) +7
class Throttler { interval_ms: u64, leading: bool, trailing: bool, has_window: bool, /* +3 fields */ } // methods: init(...) +5
```

## API Signatures

```cp
fn elapsedSince(now_ms: u64, past_ms: u64): u64;
```

## Minimal Usage

```cp
import { elapsedSince } from "lodash.debounce";

pub fn main() {
    let result = elapsedSince(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lodash.debounce"`; install via `cap add lodash.debounce`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add lodash.debounce
cpc check src/main.cp
```
