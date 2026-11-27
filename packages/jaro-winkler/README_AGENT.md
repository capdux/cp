# jaro-winkler — AI Agent Quick Ref

Zero-allocation Jaro and Jaro-Winkler string similarity metrics and best-match finder in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { jaro, jaroWinklerWithScale, jaroWinkler, bestMatchIndex } from "jaro-winkler";
```

## API Signatures

```cp
fn jaro(a: string, b: string): f64;
fn jaroWinklerWithScale(a: string, b: string, prefix_scale: f64): f64;
fn jaroWinkler(a: string, b: string): f64;
fn bestMatchIndex(query: string, candidates: &[]string, min_score: f64): i64;
```

## Minimal Usage

```cp
import { jaro } from "jaro-winkler";

pub fn main() {
    let result = jaro("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"jaro-winkler"`; install via `cap add jaro-winkler`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add jaro-winkler
cpc check src/main.cp
```
