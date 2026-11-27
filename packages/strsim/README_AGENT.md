# strsim@1.0.0 (Agent Quickref)

> Zero-heap string similarity & edit-distance metrics: Hamming, Levenshtein, OSA Damerau-Levenshtein, Jaro-Winkler, and Sorensen-Dice (clean-room port of Rust strsim) | License: MIT
> Import: `import { ... } from "strsim";` | Full types: `src/index.d.cp`

## Functions
- `export fn ssmMin3(a: usize, b: usize, c: usize): usize;`
- `export fn hamming(a: string, b: string): usize?;`
- `export fn levenshtein(a: string, b: string): usize;`
- `export fn osaDistance(a: string, b: string): usize;`
- `export fn normalizedLevenshteinBps(a: string, b: string): u32;`
- `export fn jaroBps(a: string, b: string): u32;`
- `export fn jaroWinklerBps(a: string, b: string): u32;`
- `export fn sorensenDiceBps(a: string, b: string): u32;`
