# strsim — AI Agent Quick Ref

Zero-heap string similarity & edit-distance metrics: Hamming, Levenshtein, OSA Damerau-Levenshtein, Jaro-Winkler, and Sorensen-Dice (clean-room port of Rust strsim) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ssmMin3,
    hamming,
    levenshtein,
    osaDistance,
    normalizedLevenshteinBps,
    jaroBps,
    jaroWinklerBps,
    sorensenDiceBps,
} from "strsim";
```

## API Signatures

```cp
fn ssmMin3(a: usize, b: usize, c: usize): usize;
fn hamming(a: string, b: string): usize?;
fn levenshtein(a: string, b: string): usize;
fn osaDistance(a: string, b: string): usize;
fn normalizedLevenshteinBps(a: string, b: string): u32;
fn jaroBps(a: string, b: string): u32;
fn jaroWinklerBps(a: string, b: string): u32;
fn sorensenDiceBps(a: string, b: string): u32;
```

## Minimal Usage

```cp
import { hamming } from "strsim";

pub fn main() {
    let result = hamming("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"strsim"`; install via `cap add strsim`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
