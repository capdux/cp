# picomatch@1.0.0 (Agent Quickref)

> Zero-allocation glob matcher & static base scanner (npm picomatch v4 parity) with globstar, char classes, brace expansion, dotfile protection, and negation for CP | License: MIT
> Import: `import { ... } from "picomatch";` | Full types: `src/index.d.cp`
> Deps: `is-extglob`, `normalize-path`

## Functions
- `export fn isMatchWithOptions(path: string, pattern: string, opts: PicoOptions): bool;`
- `export fn isMatch(path: string, pattern: string): bool;`
- `export fn isMatchAny(path: string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn scanBase(pattern: string, out: []u8): usize;`
