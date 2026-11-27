# locate-path — AI Agent Quick Ref

Zero-allocation path locator over candidate lists with entry-type, symlink, and ordering controls for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    LocateOptions,
    FsEntry,
    lcpStrEq,
    lcpTrimTrailingSlash,
    lcpCopyStr,
    isAbsolutePath,
    lcpResolveCandidate,
    lcpEntryMatches,
} from "locate-path";
```

## Core Types & Constants

```cp
class LocateOptions { cwd: string, entry_type: u8, allow_symlinks: bool, preserve_order: bool } // methods: defaults(...)
class FsEntry { path: string, kind: u8, is_symlink: bool }
```

## API Signatures

```cp
fn lcpStrEq(a: string, b: string): bool;
fn lcpTrimTrailingSlash(s: string): string;
fn lcpCopyStr(dst: []u8, src: string): usize;
fn isAbsolutePath(p: string): bool;
fn lcpResolveCandidate(cwd: string, cand: string, out: []u8): usize;
fn lcpEntryMatches(entry: FsEntry, target: string, opts: LocateOptions): bool;
fn locatePathInEntries(candidates: []string, entries: []FsEntry, opts: LocateOptions, out: []u8): usize?;
fn locateAllPathsInEntries(candidates: []string, entries: []FsEntry, opts: LocateOptions, matched_indices: []usize): usize;
```

## Minimal Usage

```cp
import { isAbsolutePath } from "locate-path";

pub fn main() {
    let result = isAbsolutePath("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"locate-path"`; install via `cap add locate-path`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
