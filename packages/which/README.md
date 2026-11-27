[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# which

Zero-heap PATH executable candidate resolver for POSIX and Windows (clean-room CP implementation of npm which)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:which` ([https://github.com/npm/node-which](https://github.com/npm/node-which))（原库名 `which` → CP 包名 `which`）

| Metadata | Value |
| --- | --- |
| Package | `which` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add which
```

```cp
import {
    WhichIterator,
    whEqIgnoreCase,
    isExplicitPath,
    countPathDirs,
    pathDirAt,
    hasPathExt,
    formatCandidatePath,
} from "which";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `WhichIterator` | `class` | Fields: `cmd: string, path_env: string, pathext: string, delim: u8, dir_idx: usize, dir_count: usize, +3 more` · Methods: `newPosix, newWindows, next` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `whEqIgnoreCase` | `(a: string, b: string): bool` | Invokes `whEqIgnoreCase` and returns `bool`. |
| `isExplicitPath` | `(cmd: string): bool` | Checks predicate `isExplicitPath` and returns `bool`. |
| `countPathDirs` | `(path_env: string, delim: u8): usize` | Invokes `countPathDirs` and returns `usize`. |
| `pathDirAt` | `(path_env: string, delim: u8, idx: usize, out: []u8): usize` | Invokes `pathDirAt` and returns `usize`. |
| `hasPathExt` | `(cmd: string, pathext: string): bool` | Checks predicate `hasPathExt` and returns `bool`. |
| `formatCandidatePath` | `(dir: string, cmd: string, ext: string, out: []u8): usize` | Serializes/formats data via `formatCandidatePath` returning `usize`. |

## Minimal Example

```cp
import { isExplicitPath } from "which";

pub fn main() {
    let result = isExplicitPath("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
