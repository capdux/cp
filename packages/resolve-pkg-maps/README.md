[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# resolve-pkg-maps

Zero-heap package.json exports subpath wildcard and condition resolver

| Metadata | Value |
| --- | --- |
| Package | `resolve-pkg-maps` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add resolve-pkg-maps
```

```cp
import {
    ExportRule,
    rpmEq,
    rpmFindStar,
    rpmHasParentTraversal,
    rpmConditionRank,
    matchSubpathPattern,
    resolveExportTarget,
} from "resolve-pkg-maps";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ExportRule` | `class` | Fields: `pattern: string, condition: string, target: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rpmEq` | `(a: string, b: string): bool` | Invokes `rpmEq` and returns `bool`. |
| `rpmFindStar` | `(s: string): usize` | Invokes `rpmFindStar` and returns `usize`. |
| `rpmHasParentTraversal` | `(path: string): bool` | Invokes `rpmHasParentTraversal` and returns `bool`. |
| `rpmConditionRank` | `(cond: string, conditions: &[]string): usize` | Invokes `rpmConditionRank` and returns `usize`. |
| `matchSubpathPattern` | `(pattern: string, subpath: string, matched_star: []u8): i64` | Invokes `matchSubpathPattern` and returns `i64`. |
| `resolveExportTarget` | `(rules: &[]ExportRule, subpath: string, conditions: &[]string, out: []u8): usize` | Invokes `resolveExportTarget` and returns `usize`. |

## Minimal Example

```cp
import { rpmEq } from "resolve-pkg-maps";

pub fn main() {
    let result = rpmEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
