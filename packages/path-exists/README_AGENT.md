# path-exists — AI Agent Quick Ref

Zero-heap path/file/directory existence checker with symlink awareness and batch filtering (npm path-exists equivalent, reuses locate-path) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    pathExistsInEntries,
    fileExistsInEntries,
    dirExistsInEntries,
    realPathExistsInEntries,
    countExistingPaths,
} from "path-exists";
```

## API Signatures

```cp
fn pathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;
fn fileExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;
fn dirExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;
fn realPathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;
fn countExistingPaths(targets: []string, cwd: string, entries: []FsEntry, out_indices: []usize): usize;
```

## Minimal Usage

```cp
import { pathExistsInEntries } from "path-exists";

pub fn main() {
    let result = pathExistsInEntries("example", "example", /* entries: []FsEntry */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"path-exists"`; install via `cap add path-exists`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add path-exists
cpc check src/main.cp
```
