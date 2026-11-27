# rimraf@1.0.0 (Agent Quickref)

> Zero-allocation recursive deletion safety guard, root-path validator, and bottom-up delete planner for CP, reusing fast-glob. | License: MIT
> Import: `import { ... } from "rimraf";` | Full types: `src/index.d.cp`
> Deps: `fast-glob`

## Functions
- `export fn rmfIsAsciiAlpha(b: u8): bool;`
- `export fn rmfIsSep(b: u8, is_windows: bool): bool;`
- `export fn rmfHasNul(s: string): bool;`
- `export fn rmfPathDepth(path: string, is_windows: bool): usize;`
- `export fn isRootPath(path: string, is_windows: bool): bool;`
- `export fn validateRimrafTarget(target: string, opts: RimrafOptions): bool;`
- `export fn rmfSortByDepthDesc(paths: []string, count: usize, is_windows: bool);`
- `export fn planBottomUpDelete(entries: []string, opts: RimrafOptions): RimrafPlan;`
- `export fn planFilteredDelete(entries: []string, pattern: string, opts: RimrafOptions): RimrafPlan;`
