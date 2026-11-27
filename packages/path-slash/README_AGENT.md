# path-slash@1.0.0 (Agent Quickref)

> CP package path-slash | License: MIT
> Import: `import { ... } from "path-slash";` | Full types: `src/index.d.cp`

## Functions
- `export fn pshIsAsciiAlpha(b: u8): bool;`
- `export fn pshIsSep(b: u8): bool;`
- `export fn pshWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pshWriteReplacement(out: []u8, pos: usize): usize;`
- `export fn pshValidUtf8Len(s: string, offset: usize): usize;`
- `export fn pshIsAbs(path: string): bool;`
- `export fn toSlash(path: string, out: []u8): usize;`
- `export fn toSlashLossy(path: string, out: []u8): usize;`
- `export fn fromSlash(path: string, out: []u8): usize;`
- `export fn hasBackslash(path: string): bool;`
- `export fn hasForwardSlash(path: string): bool;`
- `export fn trimTrailingSlashes(path: string): string;`
- `export fn joinSlash(a: string, b: string, out: []u8): usize;`
