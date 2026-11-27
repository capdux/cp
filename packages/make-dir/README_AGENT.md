# make-dir@1.0.0 (Agent Quickref)

> Zero-heap recursive directory creation planner with invalid-char validation, prefix chain generation, and octal permission mode (npm make-dir equivalent) | License: MIT
> Import: `import { ... } from "make-dir";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidDirPath(path: string, is_windows: bool): bool;`
- `export fn mkdStrEq(a: string, b: string): bool;`
- `export fn mkdExists(p: string, existing: []string): bool;`
- `export fn planMakeDir(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan;`
- `export fn applyUmask(mode: u32, umask: u32): u32;`
