[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# file-entry-cache

Zero-heap incremental build and lint file metadata cache in pure CP (covers flat-cache)

| Metadata | Value |
| --- | --- |
| Package | `file-entry-cache` |
| Version | `10.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add file-entry-cache
```

```cp
import {
    FileCacheEntry,
    FileEntryCache,
    fecStrEq,
    fecEmptyEntry,
    fecPathExists,
} from "file-entry-cache";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FileCacheEntry` | `class` | Fields: `path: string, mtime_ms: i64, size: u64, content_hash: u64, payload: string, valid: bool` |
| `FileEntryCache` | `class` | Fields: `entries: [64]FileCacheEntry, count: usize` · Methods: `new, set, get, hasChanged, remove, reconcile, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fecStrEq` | `(a: string, b: string): bool` | Invokes `fecStrEq` and returns `bool`. |
| `fecEmptyEntry` | `(): FileCacheEntry` | Invokes `fecEmptyEntry` and returns `FileCacheEntry`. |
| `fecPathExists` | `(path: string, existing_paths: &[]string): bool` | Invokes `fecPathExists` and returns `bool`. |

## Minimal Example

```cp
import { fecStrEq } from "file-entry-cache";

pub fn main() {
    let result = fecStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
