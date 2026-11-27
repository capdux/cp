# onetime — AI Agent Quick Ref

Ensure an initialization or action runs only once with call-count tracking (clean-room CP implementation of npm onetime) (`v1.0.0`, `MIT`)

## Import

```cp
import { OnceCallResult, OnceGuard, OnceRegistry } from "onetime";
```

## Core Types & Constants

```cp
class OnceCallResult { status: i32, executed_now: bool, value: i64, call_count: u64 }
class OnceGuard { called: bool, strict: bool, call_count: u64, cached_value: i64 } // methods: new(...) +7
class OnceRegistry { keys: [64]u64, values: [64]i64, counts: [64]u64, len: usize } // methods: new(...) +4
```

## Minimal Usage

```cp
import { OnceCallResult } from "onetime";

pub fn main() {
    // Refer to src/index.d.cp for OnceCallResult fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"onetime"`; install via `cap add onetime`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add onetime
cpc check src/main.cp
```
