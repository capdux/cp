# file-entry-cache — AI Agent Quick Ref

Zero-heap incremental build and lint file metadata cache in pure CP (covers flat-cache) (`v10.0.0`, `MIT`)

## Import

```cp
import {
    FileCacheEntry,
    FileEntryCache,
    fecStrEq,
    fecEmptyEntry,
    fecPathExists,
} from "file-entry-cache";
```

## Core Types & Constants

```cp
class FileCacheEntry { path: string, mtime_ms: i64, size: u64, content_hash: u64, /* +2 fields */ }
class FileEntryCache { entries: [64]FileCacheEntry, count: usize } // methods: new(...) +6
```

## API Signatures

```cp
fn fecStrEq(a: string, b: string): bool;
fn fecEmptyEntry(): FileCacheEntry;
fn fecPathExists(path: string, existing_paths: &[]string): bool;
```

## Minimal Usage

```cp
import { fecStrEq } from "file-entry-cache";

pub fn main() {
    let result = fecStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"file-entry-cache"`; install via `cap add file-entry-cache`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
