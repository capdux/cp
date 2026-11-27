# micromatch@1.0.0 (Agent Quickref)

> Zero-allocation glob matching suite (npm micromatch v4 parity) with brace range expansion, multi-pattern filtering, negation, and contains matching for CP | License: MIT
> Import: `import { ... } from "micromatch";` | Full types: `src/index.d.cp`
> Deps: `braces`, `picomatch`

## Functions
- `export fn mmIsMatch(path: string, pattern: string, opts: PicoOptions): bool;`
- `export fn mmAll(path: string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn mmAny(path: string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn mmSome(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn mmEvery(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn mmFilter(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize;`
- `export fn mmNot(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize;`
- `export fn mmContains(path: string, pattern: string, opts: PicoOptions): bool;`
