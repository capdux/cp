# path-exists@1.0.0 (Agent Quickref)

> Zero-heap path/file/directory existence checker with symlink awareness and batch filtering (npm path-exists equivalent, reuses locate-path) | License: MIT
> Import: `import { ... } from "path-exists";` | Full types: `src/index.d.cp`
> Deps: `locate-path`

## Functions
- `export fn pathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;`
- `export fn fileExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;`
- `export fn dirExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;`
- `export fn realPathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;`
- `export fn countExistingPaths(targets: []string, cwd: string, entries: []FsEntry, out_indices: []usize): usize;`
