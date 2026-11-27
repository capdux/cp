# is-number — AI Agent Quick Ref

Zero-heap strict numeric string validator (integers, decimals, scientific, hex) matching npm is-number in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { isNumber, isIntegerStr, isFiniteF64 } from "is-number";
```

## API Signatures

```cp
fn isNumber(s: string): bool;
fn isIntegerStr(s: string): bool;
fn isFiniteF64(x: f64): bool;
```

## Minimal Usage

```cp
import { isNumber } from "is-number";

pub fn main() {
    let result = isNumber("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"is-number"`; install via `cap add is-number`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add is-number
cpc check src/main.cp
```
