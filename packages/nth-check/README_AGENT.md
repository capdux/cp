# nth-check — AI Agent Quick Ref

Zero-allocation CSS :nth-child(An+B) microsyntax parser and index matcher (`v2.1.1`, `BSD-2-Clause`)

## Import

```cp
import { NthRule, parseNth, nthCheck } from "nth-check";
```

## Core Types & Constants

```cp
class NthRule { a: i32, b: i32, valid: bool } // methods: matches(...)
```

## API Signatures

```cp
fn parseNth(formula: string): NthRule;
fn nthCheck(formula: string, index: usize): bool;
```

## Minimal Usage

```cp
import { parseNth } from "nth-check";

pub fn main() {
    let result = parseNth("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"nth-check"`; install via `cap add nth-check`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
