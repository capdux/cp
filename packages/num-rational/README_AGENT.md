# num-rational — AI Agent Quick Ref

Zero-allocation exact rational number (Ratio64) arithmetic library with canonical reduction via num-integer for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import { Ratio64, nratAbsU64, nratReduce } from "num-rational";
```

## Core Types & Constants

```cp
class Ratio64 { numer: i64, denom: i64 } // methods: new(...) +19
```

## API Signatures

```cp
fn nratAbsU64(v: i64): u64;
fn nratReduce(numer: i64, denom: i64): Ratio64;
```

## Minimal Usage

```cp
import { nratAbsU64 } from "num-rational";

pub fn main() {
    let result = nratAbsU64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"num-rational"`; install via `cap add num-rational`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add num-rational
cpc check src/main.cp
```
