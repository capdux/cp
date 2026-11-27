[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rimraf

Zero-allocation recursive deletion safety guard, root-path validator, and bottom-up delete planner for CP, reusing fast-glob.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rimraf` ([https://github.com/isaacs/rimraf](https://github.com/isaacs/rimraf))（原库名 `rimraf` → CP 包名 `rimraf`）

| Metadata | Value |
| --- | --- |
| Package | `rimraf` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rimraf
```

```cp
import {
    RimrafOptions,
    RimrafPlan,
    rmfIsAsciiAlpha,
    rmfIsSep,
    rmfHasNul,
    rmfPathDepth,
    isRootPath,
    validateRimrafTarget,
} from "rimraf";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RimrafOptions` | `class` | Fields: `preserve_root: bool, max_retries: u8, retry_delay_ms: u32, is_windows: bool` · Methods: `defaults` |
| `RimrafPlan` | `class` | Fields: `valid: bool, paths: [32]string, count: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rmfIsAsciiAlpha` | `(b: u8): bool` | Invokes `rmfIsAsciiAlpha` and returns `bool`. |
| `rmfIsSep` | `(b: u8, is_windows: bool): bool` | Invokes `rmfIsSep` and returns `bool`. |
| `rmfHasNul` | `(s: string): bool` | Invokes `rmfHasNul` and returns `bool`. |
| `rmfPathDepth` | `(path: string, is_windows: bool): usize` | Invokes `rmfPathDepth` and returns `usize`. |
| `isRootPath` | `(path: string, is_windows: bool): bool` | Checks predicate `isRootPath` and returns `bool`. |
| `validateRimrafTarget` | `(target: string, opts: RimrafOptions): bool` | Validates input with `validateRimrafTarget` returning `bool`. |
| `rmfSortByDepthDesc` | `(paths: []string, count: usize, is_windows: bool)` | Executes `rmfSortByDepthDesc` in-place. |
| `planBottomUpDelete` | `(entries: []string, opts: RimrafOptions): RimrafPlan` | Invokes `planBottomUpDelete` and returns `RimrafPlan`. |
| `planFilteredDelete` | `(entries: []string, pattern: string, opts: RimrafOptions): RimrafPlan` | Invokes `planFilteredDelete` and returns `RimrafPlan`. |

## Minimal Example

```cp
import { isRootPath } from "rimraf";

pub fn main() {
    let result = isRootPath("example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
