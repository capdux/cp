# pkg-dir@1.0.0 (Agent Quickref)

> Zero-heap Node.js/CP package root directory finder locating nearest ancestor package.json or cap.json (npm pkg-dir equivalent, reuses @cap/find-up & @cap/npm-run-path) | License: MIT
> Import: `import { ... } from "pkg-dir";` | Full types: `src/index.d.cp`
> Deps: `find-up`, `npm-run-path`

## Functions
- `export fn pkdPush(out: []u8, pos: usize, s: string): usize;`
- `export fn packageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize?;`
- `export fn capPackageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize?;`
- `export fn packageDirectoryBounded(cwd: string, stop_at: string, existing_files: []string, out: []u8): usize?;`
