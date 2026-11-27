# is-glob — AI Agent Quick Ref

Zero-allocation glob pattern detector reusing is-extglob for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    hasClosedBracket,
    hasClosedBrace,
    hasUnescapedWildcard,
    isGlob,
    isGlobDefault,
} from "is-glob";
```

## API Signatures

```cp
fn hasClosedBracket(str: string, open_idx: usize): bool;
fn hasClosedBrace(str: string, open_idx: usize): bool;
fn hasUnescapedWildcard(str: string): bool;
fn isGlob(str: string, strict: bool): bool;
fn isGlobDefault(str: string): bool;
```

## Minimal Usage

```cp
import { isGlob } from "is-glob";

pub fn main() {
    let result = isGlob("example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"is-glob"`; install via `cap add is-glob`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add is-glob
cpc check src/main.cp
```
