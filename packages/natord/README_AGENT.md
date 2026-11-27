# natord — AI Agent Quick Ref

Zero-heap arbitrary-precision human natural sort comparator, case-insensitive comparator, and in-place sorter. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    compareImpl,
    compareNatural,
    compareNaturalIgnoreCase,
    isSortedNatural,
    sortNatural,
} from "natord";
```

## API Signatures

```cp
fn compareImpl(a: string, b: string, ignore_case: bool): i32;
fn compareNatural(a: string, b: string): i32;
fn compareNaturalIgnoreCase(a: string, b: string): i32;
fn isSortedNatural(items: &[]string): bool;
fn sortNatural(items: []string);
```

## Minimal Usage

```cp
import { isSortedNatural } from "natord";

pub fn main() {
    let result = isSortedNatural(/* items: &[]string */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"natord"`; install via `cap add natord`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add natord
cpc check src/main.cp
```
