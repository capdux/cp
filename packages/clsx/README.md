[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# clsx

Zero-heap conditional CSS class name builder with whitespace normalization, deduplication, and word-boundary checks (npm clsx/classnames parity).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:clsx` ([https://github.com/lukeed/clsx](https://github.com/lukeed/clsx))

| Metadata | Value |
| --- | --- |
| Package | `clsx` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add clsx
```

```cp
import { CondClass, ClassBuilder, joinClasses, joinCondClasses } from "clsx";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CondClass` | `class` | Fields: `name: &[]u8, enabled: bool` |
| `ClassBuilder` | `class` | Fields: `buf: []u8, pos: usize, overflow: bool` · Methods: `init, reset, has, appendToken, add, addIf, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `joinClasses` | `(classes: &[]&[]u8, dst: []u8): i64` | Invokes `joinClasses` and returns `i64`. |
| `joinCondClasses` | `(items: &[]CondClass, dst: []u8): i64` | Invokes `joinCondClasses` and returns `i64`. |

## Minimal Example

```cp
import { joinClasses } from "clsx";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = joinClasses(/* classes: &[]&[]u8 */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
