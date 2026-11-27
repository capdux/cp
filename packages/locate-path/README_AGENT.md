# locate-path@1.0.0 (Agent Quickref)

> Zero-allocation path locator over candidate lists with entry-type, symlink, and ordering controls for CP. | License: MIT
> Import: `import { ... } from "locate-path";` | Full types: `src/index.d.cp`
> Deps: `find-up`

## Functions
- `export fn lcpStrEq(a: string, b: string): bool;`
- `export fn lcpTrimTrailingSlash(s: string): string;`
- `export fn lcpCopyStr(dst: []u8, src: string): usize;`
- `export fn isAbsolutePath(p: string): bool;`
- `export fn lcpResolveCandidate(cwd: string, cand: string, out: []u8): usize;`
- `export fn lcpEntryMatches(entry: FsEntry, target: string, opts: LocateOptions): bool;`
- `export fn locatePathInEntries(candidates: []string, entries: []FsEntry, opts: LocateOptions, out: []u8): usize?;`
- `export fn locateAllPathsInEntries(candidates: []string, entries: []FsEntry, opts: LocateOptions, matched_indices: []usize): usize;`
