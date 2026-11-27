# module-details-from-path@1.0.4 (Agent Quickref)

> Zero-heap node_modules and cap_modules path parser extracting package name, basedir, and subpath | License: MIT
> Import: `import { ... } from "module-details-from-path";` | Full types: `src/index.d.cp`

## Functions
- `export fn mdfpIsSep(ch: u8): bool;`
- `export fn mdfpMatchDir(s: string, pos: usize, dir: string): bool;`
- `export fn mdfpSkipSeps(s: string, start: usize): usize;`
- `export fn mdfpFindSep(s: string, start: usize): usize;`
- `export fn mdfpEmpty(): ModuleDetails;`
- `export fn parseModulePath(file_path: string): ModuleDetails;`
