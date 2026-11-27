[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# locate-path

Zero-allocation path locator over candidate lists with entry-type, symlink, and ordering controls for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:locate-path` ([https://github.com/sindresorhus/locate-path](https://github.com/sindresorhus/locate-path))

| Metadata | Value |
| --- | --- |
| Package | `locate-path` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add locate-path
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LocateOptions` | `class` | Fields: `cwd: string, entry_type: u8, allow_symlinks: bool, preserve_order: bool` · Methods: `defaults` |
| `FsEntry` | `class` | Fields: `path: string, kind: u8, is_symlink: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lcpStrEq` | `(a: string, b: string): bool` | Invokes `lcpStrEq` and returns `bool`. |
| `lcpTrimTrailingSlash` | `(s: string): string` | Invokes `lcpTrimTrailingSlash` and returns `string`. |
| `lcpCopyStr` | `(dst: []u8, src: string): usize` | Invokes `lcpCopyStr` and returns `usize`. |
| `isAbsolutePath` | `(p: string): bool` | Checks predicate `isAbsolutePath` and returns `bool`. |
| `lcpResolveCandidate` | `(cwd: string, cand: string, out: []u8): usize` | Invokes `lcpResolveCandidate` and returns `usize`. |
| `lcpEntryMatches` | `(entry: FsEntry, target: string, opts: LocateOptions): bool` | Invokes `lcpEntryMatches` and returns `bool`. |
| `locatePathInEntries` | `(candidates: []string, entries: []FsEntry, opts: LocateOptions, out: []u8): usize?` | Invokes `locatePathInEntries` and returns `usize?`. |
| `locateAllPathsInEntries` | `(candidates: []string, entries: []FsEntry, opts: LocateOptions, matched_indices: []usize): usize` | Invokes `locateAllPathsInEntries` and returns `usize`. |

## Minimal Example

```cp
import { isAbsolutePath } from "locate-path";

pub fn main() {
    let result = isAbsolutePath("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
