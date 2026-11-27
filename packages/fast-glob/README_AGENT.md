# fast-glob@1.0.0 (Agent Quickref)

> Zero-heap pure CP fast-glob pattern set, dynamic pattern detector, and path escaper (clean-room MIT implementation of npm fast-glob v3) | License: MIT
> Import: `import { ... } from "fast-glob";` | Full types: `src/index.d.cp`
> Deps: `glob-match`

## Functions
- `export fn fglToLower(b: u8): u8;`
- `export fn fglIsGlobMeta(b: u8): bool;`
- `export fn fglNormalizeRelPath(raw: string): string;`
- `export fn fglPathDepth(path: string): usize;`
- `export fn fglHasDotSegment(path: string): bool;`
- `export fn fglLowerInto(src: string, dst: []u8): usize;`
- `export fn fglMatchOne(pattern: string, path: string, case_sensitive: bool): bool;`
- `export fn isDynamicPattern(pattern: string): bool;`
- `export fn escapePath(path: string, out: []u8): usize;`
- `export fn convertPathToPattern(path: string, out: []u8): usize;`
