# minimatch@1.0.0 (Agent Quickref)

> Minimal matching utility converting glob expressions (clean-room CP implementation of npm minimatch) | License: MIT
> Import: `import { ... } from "minimatch";` | Full types: `src/index.d.cp`
> Deps: `micromatch`, `picomatch`

## Functions
- `export fn miniToPico(opts: MinimatchOptions): PicoOptions;`
- `export fn isCommentPattern(pattern: string, opts: MinimatchOptions): bool;`
- `export fn isNegatedPattern(pattern: string, opts: MinimatchOptions): bool;`
- `export fn minimatchWithOptions(path: string, pattern: string, opts: MinimatchOptions): bool;`
- `export fn minimatch(path: string, pattern: string): bool;`
- `export fn minimatchList(paths: &[]string, pattern: string, opts: MinimatchOptions, out_indices: []usize): usize;`
- `export fn minimatchFilterAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize;`
- `export fn minimatchRejectAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize;`
