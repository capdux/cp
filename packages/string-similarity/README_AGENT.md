# string-similarity@1.0.0 (Agent Quickref)

> Zero-allocation Sorensen-Dice bigram string similarity coefficient and best-match finder for CP. | License: MIT
> Import: `import { ... } from "string-similarity";` | Full types: `src/index.d.cp`

## Functions
- `export fn diceCoefficient(a: string, b: string): f64;`
- `export fn diceCaseSensitive(a: string, b: string): f64;`
- `export fn bestMatchIndex(target: string, candidates: &[]string, min_rating: f64): i64;`
