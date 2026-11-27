# is-extglob@1.0.0 (Agent Quickref)

> Zero-allocation Bash/Micromatch extglob pattern detector (@(...), !(...), *(...), +(...), ?(...)) for CP | License: MIT
> Import: `import { ... } from "is-extglob";` | Full types: `src/index.d.cp`

## Functions
- `export fn isExtglobPrefix(ch: u8): bool;`
- `export fn matchExtglobEnd(str: string, start: usize): usize;`
- `export fn findExtglobStart(str: string): i64;`
- `export fn isExtglob(str: string): bool;`
- `export fn countExtglobs(str: string): usize;`
