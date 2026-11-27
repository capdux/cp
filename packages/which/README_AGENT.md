# which@1.0.0 (Agent Quickref)

> Zero-heap PATH executable candidate resolver for POSIX and Windows (clean-room CP implementation of npm which) | License: MIT
> Import: `import { ... } from "which";` | Full types: `src/index.d.cp`
> Deps: `normalize-path`

## Functions
- `export fn whEqIgnoreCase(a: string, b: string): bool;`
- `export fn isExplicitPath(cmd: string): bool;`
- `export fn countPathDirs(path_env: string, delim: u8): usize;`
- `export fn pathDirAt(path_env: string, delim: u8, idx: usize, out: []u8): usize;`
- `export fn hasPathExt(cmd: string, pathext: string): bool;`
- `export fn formatCandidatePath(dir: string, cmd: string, ext: string, out: []u8): usize;`
