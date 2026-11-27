[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# lilconfig

Zero-heap pure CP lightweight configuration explorer with JSON/JS defaults and custom loader registry (clean-room MIT implementation of npm lilconfig v3)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lilconfig` ([https://github.com/antonk52/lilconfig](https://github.com/antonk52/lilconfig))

| Metadata | Value |
| --- | --- |
| Package | `lilconfig` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add lilconfig
```

```cp
import {
    LilconfigOptions,
    CustomLoaderEntry,
    LilExplorer,
    lilEndsWithIgnoreCase,
    lilBasename,
    lilIsNoExt,
    formatLilSearchPlace,
    isSupportedByDefault,
} from "lilconfig";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LilconfigOptions` | `class` | Fields: `module_name: string, stop_dir: string, package_prop: string, ignore_empty_search_places: bool` · Methods: `defaults` |
| `CustomLoaderEntry` | `class` | Fields: `ext: string, loader_id: u8` |
| `LilExplorer` | `class` | Fields: `opts: LilconfigOptions, planner: SearchPlanner, custom_loaders: [8]CustomLoaderEntry, custom_count: usize` · Methods: `new, registerLoader, resolveLoader, nextSearchDir, candidatePath, makeResult` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lilEndsWithIgnoreCase` | `(s: string, suffix: string): bool` | Invokes `lilEndsWithIgnoreCase` and returns `bool`. |
| `lilBasename` | `(filepath: string): string` | Invokes `lilBasename` and returns `string`. |
| `lilIsNoExt` | `(filepath: string): bool` | Invokes `lilIsNoExt` and returns `bool`. |
| `formatLilSearchPlace` | `(module_name: string, idx: usize, out: []u8): usize` | Serializes/formats data via `formatLilSearchPlace` returning `usize`. |
| `isSupportedByDefault` | `(filepath: string): bool` | Checks predicate `isSupportedByDefault` and returns `bool`. |

## Minimal Example

```cp
import { formatLilSearchPlace } from "lilconfig";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatLilSearchPlace("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
