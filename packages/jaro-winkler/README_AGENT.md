# jaro-winkler@1.0.0 (Agent Quickref)

> Zero-allocation Jaro and Jaro-Winkler string similarity metrics and best-match finder in pure CP | License: MIT
> Import: `import { ... } from "jaro-winkler";` | Full types: `src/index.d.cp`

## Functions
- `export fn jaro(a: string, b: string): f64;`
- `export fn jaroWinklerWithScale(a: string, b: string, prefix_scale: f64): f64;`
- `export fn jaroWinkler(a: string, b: string): f64;`
- `export fn bestMatchIndex(query: string, candidates: &[]string, min_score: f64): i64;`
