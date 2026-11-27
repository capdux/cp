# on-finished — AI Agent Quick Ref

Zero-heap HTTP message/stream completion tracker with once-only finish/error/close lifecycle dispatch (npm on-finished equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import { ListenerSlot, StreamLifecycle, isFinished } from "on-finished";
```

## Core Types & Constants

```cp
class ListenerSlot { id: u32, fired: bool, reason: u8, err_code: i32 }
class StreamLifecycle { readable_ended: bool, writable_finished: bool, socket_closed: bool, errored: bool, /* +5 fields */ } // methods: new(...) +10
```

## API Signatures

```cp
fn isFinished(lc: StreamLifecycle): bool;
```

## Minimal Usage

```cp
import { isFinished } from "on-finished";

pub fn main() {
    let result = isFinished(/* lc: StreamLifecycle */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"on-finished"`; install via `cap add on-finished`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add on-finished
cpc check src/main.cp
```
