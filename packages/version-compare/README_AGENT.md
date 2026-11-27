# version-compare@1.0.0 (Agent Quickref)

> CP package version-compare | License: MIT
> Import: `import { ... } from "version-compare";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum VersionCmp`

## Functions
- `export fn vcIsSep(b: u8): bool;`
- `export fn vcTagRank(s: string, start: usize, len: usize): i32;`
- `export fn vcCmpLexLower(a: string, aStart: usize, aLen: usize, b: string, bStart: usize, bLen: usize): i32;`
- `export fn vcParse(raw: string): VcParsed;`
- `export fn compareVersions(a: string, b: string): VersionCmp;`
- `export fn isLess(a: string, b: string): bool;`
- `export fn isEqual(a: string, b: string): bool;`
- `export fn isGreater(a: string, b: string): bool;`
- `export fn satisfies(ver: string, op: string, target: string): bool;`
