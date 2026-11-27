# read-pkg-up@1.0.0 (Agent Quickref)

> Zero-allocation upward package.json finder, parser, and normalizer for CP, reusing find-up and read-pkg. | License: MIT
> Import: `import { ... } from "read-pkg-up";` | Full types: `src/index.d.cp`
> Deps: `find-up`, `read-pkg`

## Functions
- `export fn rpuStrEq(a: string, b: string): bool;`
- `export fn rpuCopyBytes(out: []u8, src: string): usize;`
- `export fn readPackageUpInEntries(cwd: string, stop_at: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult;`
