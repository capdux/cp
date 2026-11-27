# test-exclude@7.0.1 (Agent Quickref)

> Zero-heap test and coverage file path glob filter engine | License: ISC
> Import: `import { ... } from "test-exclude";` | Full types: `src/index.d.cp`

## Functions
- `export fn texcHasPrefix(s: string, prefix: string): bool;`
- `export fn texcHasSuffix(s: string, suffix: string): bool;`
- `export fn texcContainsSegment(path: string, seg: string): bool;`
- `export fn texcMatchAt(pat: string, pi: usize, text: string, ti: usize): bool;`
- `export fn texcMatchRaw(pat: string, path: string): bool;`
- `export fn matchSimpleGlob(pattern: string, path: string): bool;`
- `export fn texcMatchesExt(path: string, ext: string): bool;`
- `export fn shouldInstrument(rel_path: string, cfg: ExcludeConfig): bool;`
