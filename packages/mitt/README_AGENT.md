# mitt — AI Agent Quick Ref

Zero-allocation tiny functional event emitter with wildcard '*' support in pure CP (npm mitt v3 parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MittListener,
    MittDispatch,
    Emitter,
    mitStrEq,
    mitIsWildcard,
    mitt,
} from "mitt";
```

## Core Types & Constants

```cp
class MittListener { active: bool, event_type: string, handler_id: u32, is_wildcard: bool } // methods: empty(...)
class MittDispatch { handler_id: u32, payload: i64, is_wildcard: bool } // methods: empty(...)
class Emitter { listeners: [32]MittListener, count: usize, last_dispatched: [32]MittDispatch, last_dispatch_count: usize, /* +1 fields */ } // methods: new(...) +7
```

## API Signatures

```cp
fn mitStrEq(a: string, b: string): bool;
fn mitIsWildcard(event_type: string): bool;
fn mitt(): Emitter;
```

## Minimal Usage

```cp
import { mitStrEq } from "mitt";

pub fn main() {
    let result = mitStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mitt"`; install via `cap add mitt`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
