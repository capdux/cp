# dunce@1.0.0 (Agent Quickref)

> CP package dunce | License: MIT
> Import: `import { ... } from "dunce";` | Full types: `src/index.d.cp`

## Functions
- `export fn isIllegalWinChar(ch: u8): bool;`
- `export fn isWindowsReservedName(component: string): bool;`
- `export fn hasWindowsVerbatimPrefix(path: string): bool;`
- `export fn canStripVerbatimPrefix(path: string): bool;`
- `export fn simplified(path: string, out: []u8): usize;`
