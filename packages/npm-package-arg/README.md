[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# npm-package-arg

Zero-heap npm package name validator and package specifier (@scope/name@version) parser and formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`npm-package-arg`](https://github.com/npm/npm-package-arg)（`ISC`）（原库名 `npm-package-arg`，CP 包名 `npm-package-arg`）。

| Metadata | Value |
| --- | --- |
| Package | `npm-package-arg` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add npm-package-arg
```

```cp
import {
    PackageSpec,
    isValidSegment,
    isValidPackageName,
    parsePackageSpec,
    appendSlice,
    formatPackageSpec,
} from "npm-package-arg";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PackageSpec` | `class` | Fields: `scope: string, name: string, full_name: string, version_spec: string, is_scoped: bool, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isValidSegment` | `(seg: string): bool` | Checks predicate `isValidSegment` and returns `bool`. |
| `isValidPackageName` | `(name: string): bool` | Checks predicate `isValidPackageName` and returns `bool`. |
| `parsePackageSpec` | `(input: string): PackageSpec` | Parses input via `parsePackageSpec` and returns `PackageSpec`. |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | Invokes `appendSlice` and returns `i64`. |
| `formatPackageSpec` | `(spec: PackageSpec, out: []u8): usize` | Serializes/formats data via `formatPackageSpec` returning `usize`. |

## Minimal Example

```cp
import { isValidSegment } from "npm-package-arg";

pub fn main() {
    let result = isValidSegment("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
