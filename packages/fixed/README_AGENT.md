# fixed — AI Agent Quick Ref

Zero-allocation signed 64-bit Q32.32 deterministic fixed-point arithmetic engine with overflow-free 128-bit split mul/div, Newton sqrt, lerp, and clamp (Rust fixed parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { Fixed32, absU64, divShift32U64 } from "fixed";
```

## Core Types & Constants

```cp
class Fixed32 { raw: i64 } // methods: fromInt(...) +19
```

## API Signatures

```cp
fn absU64(v: i64): u64;
fn divShift32U64(a: u64, b: u64): u64;
```

## Minimal Usage

```cp
import { absU64 } from "fixed";

pub fn main() {
    let result = absU64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fixed"`; install via `cap add fixed`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add fixed
cpc check src/main.cp
```
