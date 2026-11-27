# diff-sequences — AI Agent Quick Ref

Zero-heap Longest Common Subsequence (LCS) length, diff insert/delete stats, LCS similarity, and Longest Common Substring locator. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SubstringMatch,
    DiffStats,
    lcsLength,
    diffStats,
    lcsSimilarity,
    longestCommonSubstring,
} from "diff-sequences";
```

## Core Types & Constants

```cp
class SubstringMatch { a_start: usize, b_start: usize, len: usize }
class DiffStats { common: usize, insertions: usize, deletions: usize }
```

## API Signatures

```cp
fn lcsLength(a: string, b: string): usize;
fn diffStats(a: string, b: string): DiffStats;
fn lcsSimilarity(a: string, b: string): f64;
fn longestCommonSubstring(a: string, b: string): SubstringMatch;
```

## Minimal Usage

```cp
import { lcsLength } from "diff-sequences";

pub fn main() {
    let result = lcsLength("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"diff-sequences"`; install via `cap add diff-sequences`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
