# resolve-from@1.0.0 (Agent Quickref)

> Zero-heap Node.js/CP module specifier resolver from a base directory with relative, scoped, and ancestor node_modules/cap_modules probing (npm resolve-from equivalent) | License: MIT
> Import: `import { ... } from "resolve-from";` | Full types: `src/index.d.cp`
> Deps: `find-up`, `npm-run-path`

## Functions
- `export fn rfExists(path: string, files: []string): bool;`
- `export fn isRelativeSpecifier(spec: string): bool;`
- `export fn isScopedSpecifier(spec: string): bool;`
- `export fn rfProbeBase(base: string, files: []string, out: []u8): usize?;`
- `export fn rfResolveRelativeBase(from_dir: string, rel: string, out: []u8): usize;`
- `export fn resolveFrom(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?;`
