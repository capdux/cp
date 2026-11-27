# didyoumean — AI Agent Quick Ref

Zero-allocation spell-suggestion & edit-distance engine (didyoumean / fastest-levenshtein / Rust strsim parity: Levenshtein, Damerau-Levenshtein, Jaro-Winkler & CLI command suggestion) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Suggestion,
    min3,
    levenshtein,
    damerauLevenshtein,
    similarity,
    didYouMeanWith,
    didYouMean,
} from "didyoumean";
```

## Core Types & Constants

```cp
class Suggestion { found: bool, match_str: string, distance: usize, index: usize }
```

## API Signatures

```cp
fn min3(a: usize, b: usize, c: usize): usize;
fn levenshtein(a: string, b: string): usize;
fn damerauLevenshtein(a: string, b: string, ignore_case: bool): usize;
fn similarity(a: string, b: string): f64;
fn didYouMeanWith(input: string, candidates: []string, max_dist: usize): Suggestion;
fn didYouMean(input: string, candidates: []string): string?;
```

## Minimal Usage

```cp
import { levenshtein } from "didyoumean";

pub fn main() {
    let result = levenshtein("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"didyoumean"`; install via `cap add didyoumean`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
