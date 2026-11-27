# p-locate — AI Agent Quick Ref

Zero-allocation bounded-concurrency ordered/unordered locator planner (npm p-locate v7 equivalent) for CP, reusing p-limit. (`v1.0.0`, `MIT`)

## Import

```cp
import { PLocateOptions, LocateCandidate, PLocatePlanner, plocEmptyCandidate } from "p-locate";
```

## Core Types & Constants

```cp
class PLocateOptions { concurrency: usize, preserve_order: bool } // methods: defaults(...)
class LocateCandidate { id: u32, matched: bool, finished: bool }
class PLocatePlanner { opts: PLocateOptions, limiter: PLimit, items: [32]LocateCandidate, count: usize, /* +3 fields */ } // methods: new(...) +6
```

## API Signatures

```cp
fn plocEmptyCandidate(): LocateCandidate;
```

## Minimal Usage

```cp
import { plocEmptyCandidate } from "p-locate";

pub fn main() {
    let result = plocEmptyCandidate();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"p-locate"`; install via `cap add p-locate`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
