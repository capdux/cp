[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cosmiconfig

Zero-heap pure CP configuration file search planner, candidate path builder, and loader detector (clean-room MIT implementation of npm cosmiconfig v9)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cosmiconfig` ([https://github.com/cosmiconfig/cosmiconfig](https://github.com/cosmiconfig/cosmiconfig))

| Metadata | Value |
| --- | --- |
| Package | `cosmiconfig` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cosmiconfig
```

```cp
import {
    ExplorerOptions,
    SearchPlanner,
    CosmiConfigResult,
    cosmiEndsWithIgnoreCase,
    cosmiBasename,
    cosmiTrimTrailingSlash,
    detectLoader,
    formatSearchPlace,
} from "cosmiconfig";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ExplorerOptions` | `class` | Fields: `module_name: string, stop_dir: string, package_prop: string, ignore_empty: bool` · Methods: `defaults` |
| `SearchPlanner` | `class` | Fields: `start_dir: string, stop_dir: string, current_end: usize, done: bool` · Methods: `new, nextDir, dirCount` |
| `CosmiConfigResult` | `class` | Fields: `filepath: string, is_empty: bool, loader_kind: u8, found: bool` · Methods: `notFound, hit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cosmiEndsWithIgnoreCase` | `(s: string, suffix: string): bool` | Invokes `cosmiEndsWithIgnoreCase` and returns `bool`. |
| `cosmiBasename` | `(filepath: string): string` | Invokes `cosmiBasename` and returns `string`. |
| `cosmiTrimTrailingSlash` | `(dir: string): string` | Invokes `cosmiTrimTrailingSlash` and returns `string`. |
| `detectLoader` | `(filepath: string): u8` | Invokes `detectLoader` and returns `u8`. |
| `formatSearchPlace` | `(module_name: string, place_idx: usize, out: []u8): usize` | Serializes/formats data via `formatSearchPlace` returning `usize`. |
| `buildCandidatePath` | `(dir: string, place: string, out: []u8): usize` | Invokes `buildCandidatePath` and returns `usize`. |

## Minimal Example

```cp
import { formatSearchPlace } from "cosmiconfig";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatSearchPlace("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
