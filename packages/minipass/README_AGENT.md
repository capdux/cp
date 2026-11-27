# minipass — AI Agent Quick Ref

Zero-allocation fixed-capacity ring-buffer byte stream with synchronous backpressure and piping for CP (`v1.0.0`, `ISC`)

## Import

```cp
import { MinipassStream, mpassMin } from "minipass";
```

## Core Types & Constants

```cp
class MinipassStream { buf: [4096]u8, head: usize, len: usize, flowing: bool, /* +2 fields */ } // methods: new(...) +14
```

## API Signatures

```cp
fn mpassMin(a: usize, b: usize): usize;
```

## Minimal Usage

```cp
import { mpassMin } from "minipass";

pub fn main() {
    let result = mpassMin(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"minipass"`; install via `cap add minipass`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add minipass
cpc check src/main.cp
```
