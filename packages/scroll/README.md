[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# scroll

Zero-allocation endian-aware offset-advancing binary reader/writer cursors (PreadCursor & PwriteCursor) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:scroll` ([https://github.com/m4b/scroll](https://github.com/m4b/scroll)) / `crates.io:endian-type` ([https://github.com/dpc/endian-type](https://github.com/dpc/endian-type))

| Metadata | Value |
| --- | --- |
| Package | `scroll` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add scroll
```

```cp
import { Endian, PreadCursor, PwriteCursor } from "scroll";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `Endian` | `Little, Big` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PreadCursor` | `class` | Fields: `buf: &[]u8, offset: usize` · Methods: `new, remaining, readU8, readU16, readU32, readU64, +2 more` |
| `PwriteCursor` | `class` | Fields: `buf: []u8, offset: usize` · Methods: `new, written, remaining, writeU8, writeU16, writeU32, +1 more` |

## Minimal Example

```cp
import { PreadCursor } from "scroll";

pub fn main() {
    // Refer to src/index.d.cp for PreadCursor fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
