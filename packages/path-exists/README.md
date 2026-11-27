[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# path-exists

Zero-heap path/file/directory existence checker with symlink awareness and batch filtering (npm path-exists equivalent, reuses locate-path)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:path-exists` ([https://github.com/sindresorhus/path-exists](https://github.com/sindresorhus/path-exists))

| Metadata | Value |
| --- | --- |
| Package | `path-exists` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add path-exists
```

```cp
import {
    pathExistsInEntries,
    fileExistsInEntries,
    dirExistsInEntries,
    realPathExistsInEntries,
    countExistingPaths,
} from "path-exists";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pathExistsInEntries` | `(target: string, cwd: string, entries: []FsEntry): bool` | Invokes `pathExistsInEntries` and returns `bool`. |
| `fileExistsInEntries` | `(target: string, cwd: string, entries: []FsEntry): bool` | Invokes `fileExistsInEntries` and returns `bool`. |
| `dirExistsInEntries` | `(target: string, cwd: string, entries: []FsEntry): bool` | Invokes `dirExistsInEntries` and returns `bool`. |
| `realPathExistsInEntries` | `(target: string, cwd: string, entries: []FsEntry): bool` | Invokes `realPathExistsInEntries` and returns `bool`. |
| `countExistingPaths` | `(targets: []string, cwd: string, entries: []FsEntry, out_indices: []usize): usize` | Invokes `countExistingPaths` and returns `usize`. |

## Minimal Example

```cp
import { pathExistsInEntries } from "path-exists";

pub fn main() {
    let result = pathExistsInEntries("example", "example", /* entries: []FsEntry */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
