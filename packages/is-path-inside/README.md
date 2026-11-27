[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# is-path-inside

Zero-allocation path traversal guard checking whether child_path is strictly inside parent_path

| Metadata | Value |
| --- | --- |
| Package | `is-path-inside` |
| Version | `4.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add is-path-inside
```

```cp
import { ipiIsSep, ipiNormalize, isPathInside, isPathInsideOrEqual } from "is-path-inside";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ipiIsSep` | `(b: u8): bool` | Invokes `ipiIsSep` and returns `bool`. |
| `ipiNormalize` | `(input: string, out: []u8): usize` | Invokes `ipiNormalize` and returns `usize`. |
| `isPathInside` | `(child_path: string, parent_path: string): bool` | Checks predicate `isPathInside` and returns `bool`. |
| `isPathInsideOrEqual` | `(child_path: string, parent_path: string): bool` | Checks predicate `isPathInsideOrEqual` and returns `bool`. |

## Minimal Example

```cp
import { isPathInside } from "is-path-inside";

pub fn main() {
    let result = isPathInside("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
