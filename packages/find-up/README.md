[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# find-up

Zero-heap upward ancestor directory file/directory search walker with stopAt boundary and multi-name candidate generation (npm find-up equivalent, reuses npm-run-path)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:find-up` ([https://github.com/sindresorhus/find-up](https://github.com/sindresorhus/find-up))

| Metadata | Value |
| --- | --- |
| Package | `find-up` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add find-up
```

```cp
import {
    FindUpOptions,
    FindUpWalker,
    fupStrEq,
    fupTrimTrailingSlash,
    fupPush,
    joinDirAndName,
    findUpInList,
    findUpMultipleInList,
} from "find-up";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FindUpOptions` | `class` | Fields: `cwd: string, stop_at: string, allow_symlinks: bool` · Methods: `defaults` |
| `FindUpWalker` | `class` | Fields: `current: string, stop_at: string, done: bool, steps: usize` · Methods: `new, nextDir, nextCandidate` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fupStrEq` | `(a: string, b: string): bool` | Invokes `fupStrEq` and returns `bool`. |
| `fupTrimTrailingSlash` | `(s: string): string` | Invokes `fupTrimTrailingSlash` and returns `string`. |
| `fupPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `fupPush` and returns `usize`. |
| `joinDirAndName` | `(dir: string, name: string, out: []u8): usize` | Invokes `joinDirAndName` and returns `usize`. |
| `findUpInList` | `(opts: FindUpOptions, filename: string, existing_paths: []string, out: []u8): usize?` | Queries/retrieves data via `findUpInList` returning `usize?`. |
| `findUpMultipleInList` | `(opts: FindUpOptions, filenames: []string, existing_paths: []string, out: []u8): usize?` | Queries/retrieves data via `findUpMultipleInList` returning `usize?`. |

## Minimal Example

```cp
import { fupStrEq } from "find-up";

pub fn main() {
    let result = fupStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
