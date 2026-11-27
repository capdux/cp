# ee-first — AI Agent Quick Ref

Zero-heap pure CP multi-emitter first-event race listener with automatic cleanup (clean-room MIT implementation of npm ee-first v1) (`v1.0.0`, `MIT`)

## Import

```cp
import { EventWatch, FirstResult, FirstRace, eefStrEq } from "ee-first";
```

## Core Types & Constants

```cp
class EventWatch { emitter_id: u32, event_name: string, active: bool }
class FirstResult { triggered: bool, emitter_id: u32, event_name: string, payload_code: i32, /* +1 fields */ } // methods: empty(...)
class FirstRace { watchers: [16]EventWatch, count: usize, settled: bool, result: FirstResult } // methods: new(...) +6
```

## API Signatures

```cp
fn eefStrEq(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { eefStrEq } from "ee-first";

pub fn main() {
    let result = eefStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ee-first"`; install via `cap add ee-first`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
