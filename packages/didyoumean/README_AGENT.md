# didyoumean@1.0.0 (Agent Quickref)

> Zero-allocation spell-suggestion & edit-distance engine (didyoumean / fastest-levenshtein / Rust strsim parity: Levenshtein, Damerau-Levenshtein, Jaro-Winkler & CLI command suggestion) for CP | License: MIT
> Import: `import { ... } from "didyoumean";` | Full types: `src/index.d.cp`

## Functions
- `export fn levenshtein(a: string, b: string): usize;`
- `export fn damerauLevenshtein(a: string, b: string, ignore_case: bool): usize;`
- `export fn similarity(a: string, b: string): f64;`
- `export fn didYouMeanWith(input: string, candidates: []string, max_dist: usize): Suggestion;`
- `export fn didYouMean(input: string, candidates: []string): string?;`
