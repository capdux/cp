# anymatch@1.0.0 (Agent Quickref)

> CP package anymatch | License: MIT
> Import: `import { ... } from "anymatch";` | Full types: `src/index.d.cp`
> Deps: `is-glob`, `normalize-path`

## Functions
- `export fn bytesEq(a: &[]u8, b: &[]u8): bool;`
- `export fn startsWithDirPrefix(path: &[]u8, prefix: &[]u8): bool;`
- `export fn globMatchAt(pat: &[]u8, pi_in: usize, txt: &[]u8, ti_in: usize): bool;`
- `export fn matchPattern(pattern: string, test_path: string): bool;`
- `export fn anymatchIndex(patterns: &[]string, test_path: string): i64;`
- `export fn anymatch(patterns: &[]string, test_path: string): bool;`
- `export fn countGlobPatterns(patterns: &[]string): usize;`
