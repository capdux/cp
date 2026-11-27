# pathdiff@1.0.0 (Agent Quickref)

> Zero-heap relative path difference calculator computing path relative to a base directory (crates.io pathdiff equivalent) | License: MIT
> Import: `import { ... } from "pathdiff";` | Full types: `src/index.d.cp`

## Functions
- `export fn pdStrEq(a: string, b: string): bool;`
- `export fn pdPush(out: []u8, pos: usize, s: string): usize;`
- `export fn pdSplitNormalized(path: string): SegList;`
- `export fn diffPaths(target: string, base: string, out: []u8): usize?;`
