# is-glob@1.0.0 (Agent Quickref)

> Zero-allocation glob pattern detector reusing is-extglob for CP | License: MIT
> Import: `import { ... } from "is-glob";` | Full types: `src/index.d.cp`
> Deps: `is-extglob`

## Functions
- `export fn hasClosedBracket(str: string, open_idx: usize): bool;`
- `export fn hasClosedBrace(str: string, open_idx: usize): bool;`
- `export fn hasUnescapedWildcard(str: string): bool;`
- `export fn isGlob(str: string, strict: bool): bool;`
- `export fn isGlobDefault(str: string): bool;`
