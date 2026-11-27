[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# npm-run-path

Zero-heap PATH builder that prepends ancestor node_modules/.bin (and cap_modules/.bin) directories for local binary execution (npm npm-run-path equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:npm-run-path` ([https://github.com/sindresorhus/npm-run-path](https://github.com/sindresorhus/npm-run-path))

| Metadata | Value |
| --- | --- |
| Package | `npm-run-path` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add npm-run-path
```

```cp
import {
    RunPathOptions,
    nrpPushBytes,
    nrpPushChar,
    nrpIsSep,
    dirnameOf,
    countAncestorDirs,
    buildRunPath,
    pathKey,
} from "npm-run-path";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RunPathOptions` | `class` | Fields: `cwd: string, path_env: string, exec_path: string, prefer_local: bool, add_exec_path: bool, is_windows: bool, +1 more` · Methods: `defaults, forCap` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nrpPushBytes` | `(out: []u8, pos: usize, s: string): usize` | Invokes `nrpPushBytes` and returns `usize`. |
| `nrpPushChar` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `nrpPushChar` and returns `usize`. |
| `nrpIsSep` | `(c: u8, is_windows: bool): bool` | Invokes `nrpIsSep` and returns `bool`. |
| `dirnameOf` | `(path: string, is_windows: bool): string` | Invokes `dirnameOf` and returns `string`. |
| `countAncestorDirs` | `(cwd: string, is_windows: bool): usize` | Invokes `countAncestorDirs` and returns `usize`. |
| `buildRunPath` | `(opts: RunPathOptions, out: []u8): usize` | Invokes `buildRunPath` and returns `usize`. |
| `pathKey` | `(is_windows: bool): string` | Invokes `pathKey` and returns `string`. |

## Minimal Example

```cp
import { nrpPushBytes } from "npm-run-path";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = nrpPushBytes(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
