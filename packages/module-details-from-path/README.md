[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# module-details-from-path

Zero-heap node_modules and cap_modules path parser extracting package name, basedir, and subpath

| Metadata | Value |
| --- | --- |
| Package | `module-details-from-path` |
| Version | `1.0.4` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add module-details-from-path
```

```cp
import {
    ModuleDetails,
    mdfpIsSep,
    mdfpMatchDir,
    mdfpSkipSeps,
    mdfpFindSep,
    mdfpEmpty,
    parseModulePath,
} from "module-details-from-path";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ModuleDetails` | `class` | Fields: `name: string, basedir: string, subpath: string, found: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mdfpIsSep` | `(ch: u8): bool` | Invokes `mdfpIsSep` and returns `bool`. |
| `mdfpMatchDir` | `(s: string, pos: usize, dir: string): bool` | Invokes `mdfpMatchDir` and returns `bool`. |
| `mdfpSkipSeps` | `(s: string, start: usize): usize` | Invokes `mdfpSkipSeps` and returns `usize`. |
| `mdfpFindSep` | `(s: string, start: usize): usize` | Invokes `mdfpFindSep` and returns `usize`. |
| `mdfpEmpty` | `(): ModuleDetails` | Invokes `mdfpEmpty` and returns `ModuleDetails`. |
| `parseModulePath` | `(file_path: string): ModuleDetails` | Parses input via `parseModulePath` and returns `ModuleDetails`. |

## Minimal Example

```cp
import { parseModulePath } from "module-details-from-path";

pub fn main() {
    let result = parseModulePath("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
