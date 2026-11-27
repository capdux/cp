# normalize-path@1.0.0 (Agent Quickref)

> CP package normalize-path | License: MIT
> Import: `import { ... } from "normalize-path";` | Full types: `src/index.d.cp`

## Functions
- `export fn isSlash(ch: u8): bool;`
- `export fn isAsciiAlpha(ch: u8): bool;`
- `export fn hasBackslash(path: string): bool;`
- `export fn isUncPath(path: string): bool;`
- `export fn normalizePath(path: string, strip_trailing: bool, out: []u8): usize;`
- `export fn normalizePathDefault(path: string, out: []u8): usize;`
