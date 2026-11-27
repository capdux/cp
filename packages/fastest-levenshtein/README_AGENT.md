# fastest-levenshtein@1.0.0 (Agent Quickref)

> Zero-heap Levenshtein edit distance, OSA Damerau-Levenshtein transposition distance, normalized similarity, and closest-match lookup. | License: MIT
> Import: `import { ... } from "fastest-levenshtein";` | Full types: `src/index.d.cp`

## Functions
- `export fn absDiff(a: usize, b: usize): usize;`
- `export fn min3(a: usize, b: usize, c: usize): usize;`
- `export fn distance(a: string, b: string): usize;`
- `export fn damerauDistance(a: string, b: string): usize;`
- `export fn similarity(a: string, b: string): f64;`
- `export fn withinDistance(a: string, b: string, max_dist: usize): bool;`
- `export fn closestIndex(query: string, candidates: &[]string, max_dist: usize): i64;`
