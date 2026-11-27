# lilconfig@1.0.0 (Agent Quickref)

> Zero-heap pure CP lightweight configuration explorer with JSON/JS defaults and custom loader registry (clean-room MIT implementation of npm lilconfig v3) | License: MIT
> Import: `import { ... } from "lilconfig";` | Full types: `src/index.d.cp`
> Deps: `cosmiconfig`

## Functions
- `export fn lilToLower(b: u8): u8;`
- `export fn lilEqIgnoreCase(a: string, b: string): bool;`
- `export fn lilEndsWithIgnoreCase(s: string, suffix: string): bool;`
- `export fn lilWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn lilBasename(filepath: string): string;`
- `export fn lilIsNoExt(filepath: string): bool;`
- `export fn formatLilSearchPlace(module_name: string, idx: usize, out: []u8): usize;`
- `export fn isSupportedByDefault(filepath: string): bool;`
