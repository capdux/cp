# scroll — AI Agent Quick Ref

Zero-allocation endian-aware offset-advancing binary reader/writer cursors (PreadCursor & PwriteCursor) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { Endian, PreadCursor, PwriteCursor } from "scroll";
```

## Core Types & Constants

```cp
enum Endian { Little, Big }
class PreadCursor { buf: &[]u8, offset: usize } // methods: new(...) +7
class PwriteCursor { buf: []u8, offset: usize } // methods: new(...) +6
```

## Minimal Usage

```cp
import { PreadCursor } from "scroll";

pub fn main() {
    // Refer to src/index.d.cp for PreadCursor fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"scroll"`; install via `cap add scroll`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add scroll
cpc check src/main.cp
```
