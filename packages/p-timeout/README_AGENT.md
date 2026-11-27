# p-timeout — AI Agent Quick Ref

Zero-allocation deadline tracker and timeout state machine guard (npm p-timeout v6 equivalent) for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import { TimeoutOptions, TimeoutGuard, ptoMaxU64, ptoSaturatingAdd } from "p-timeout";
```

## Core Types & Constants

```cp
class TimeoutOptions { milliseconds: u64, has_fallback: bool, fallback_code: i32, error_message: string } // methods: withMs(...)
class TimeoutGuard { start_ms: u64, deadline_ms: u64, opts: TimeoutOptions, state: u8, /* +2 fields */ } // methods: new(...) +8
```

## API Signatures

```cp
fn ptoMaxU64(): u64;
fn ptoSaturatingAdd(a: u64, b: u64): u64;
```

## Minimal Usage

```cp
import { ptoMaxU64 } from "p-timeout";

pub fn main() {
    let result = ptoMaxU64();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"p-timeout"`; install via `cap add p-timeout`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
