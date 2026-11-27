# fastest-levenshtein — AI Agent Quick Ref

Zero-heap Levenshtein edit distance, OSA Damerau-Levenshtein transposition distance, normalized similarity, and closest-match lookup. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    absDiff,
    min3,
    distance,
    damerauDistance,
    similarity,
    withinDistance,
    closestIndex,
} from "fastest-levenshtein";
```

## API Signatures

```cp
fn absDiff(a: usize, b: usize): usize;
fn min3(a: usize, b: usize, c: usize): usize;
fn distance(a: string, b: string): usize;
fn damerauDistance(a: string, b: string): usize;
fn similarity(a: string, b: string): f64;
fn withinDistance(a: string, b: string, max_dist: usize): bool;
fn closestIndex(query: string, candidates: &[]string, max_dist: usize): i64;
```

## Minimal Usage

```cp
import { distance } from "fastest-levenshtein";

pub fn main() {
    let result = distance("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fastest-levenshtein"`; install via `cap add fastest-levenshtein`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add fastest-levenshtein
cpc check src/main.cp
```
