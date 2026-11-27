# find-up@1.0.0 (Agent Quickref)

> Zero-heap upward ancestor directory file/directory search walker with stopAt boundary and multi-name candidate generation (npm find-up equivalent, reuses npm-run-path) | License: MIT
> Import: `import { ... } from "find-up";` | Full types: `src/index.d.cp`
> Deps: `npm-run-path`

## Functions
- `export fn fupStrEq(a: string, b: string): bool;`
- `export fn fupTrimTrailingSlash(s: string): string;`
- `export fn fupPush(out: []u8, pos: usize, s: string): usize;`
- `export fn joinDirAndName(dir: string, name: string, out: []u8): usize;`
- `export fn findUpInList(opts: FindUpOptions, filename: string, existing_paths: []string, out: []u8): usize?;`
- `export fn findUpMultipleInList(opts: FindUpOptions, filenames: []string, existing_paths: []string, out: []u8): usize?;`
