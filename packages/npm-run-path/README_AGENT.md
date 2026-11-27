# npm-run-path@1.0.0 (Agent Quickref)

> Zero-heap PATH builder that prepends ancestor node_modules/.bin (and cap_modules/.bin) directories for local binary execution (npm npm-run-path equivalent) | License: MIT
> Import: `import { ... } from "npm-run-path";` | Full types: `src/index.d.cp`

## Functions
- `export fn nrpPushBytes(out: []u8, pos: usize, s: string): usize;`
- `export fn nrpPushChar(out: []u8, pos: usize, c: u8): usize;`
- `export fn nrpIsSep(c: u8, is_windows: bool): bool;`
- `export fn dirnameOf(path: string, is_windows: bool): string;`
- `export fn countAncestorDirs(cwd: string, is_windows: bool): usize;`
- `export fn buildRunPath(opts: RunPathOptions, out: []u8): usize;`
- `export fn pathKey(is_windows: bool): string;`
