# yn — AI Agent Quick Ref

Zero-allocation parser for yes/no boolean strings (y/yes/true/1/on/n/no/false/0/off) with lenient mode (`v5.0.0`, `MIT`)

## Import

```cp
import { yn, ynLenient, ynDefault } from "yn";
```

## API Signatures

```cp
fn yn(input: string): bool?;
fn ynLenient(input: string): bool?;
fn ynDefault(input: string, default_val: bool): bool;
```

## Minimal Usage

```cp
import { yn } from "yn";

pub fn main() {
    let result = yn("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"yn"`; install via `cap add yn`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add yn
cpc check src/main.cp
```
