[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# merge-descriptors

Zero-allocation property descriptor table and safe descriptor merge engine for CP (clean-room parity with npm merge-descriptors v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:merge-descriptors` ([https://github.com/component/merge-descriptors](https://github.com/component/merge-descriptors))

| Metadata | Value |
| --- | --- |
| Package | `merge-descriptors` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add merge-descriptors
```

```cp
import {
    PropDescriptor,
    DescriptorTable,
    mgdStrEq,
    mgdEmptyDesc,
    mergeDescriptors,
    countEnumerable,
} from "merge-descriptors";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PropDescriptor` | `class` | Fields: `key: string, value: string, writable: bool, enumerable: bool, configurable: bool` · Methods: `data, readonly` |
| `DescriptorTable` | `class` | Fields: `entries: [32]PropDescriptor, count: usize` · Methods: `new, len, has, get, define, deleteKey` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mgdStrEq` | `(a: string, b: string): bool` | Invokes `mgdStrEq` and returns `bool`. |
| `mgdEmptyDesc` | `(): PropDescriptor` | Invokes `mgdEmptyDesc` and returns `PropDescriptor`. |
| `mergeDescriptors` | `(dst: &mut DescriptorTable, src: DescriptorTable, redefine: bool): usize` | Invokes `mergeDescriptors` and returns `usize`. |
| `countEnumerable` | `(tbl: DescriptorTable): usize` | Invokes `countEnumerable` and returns `usize`. |

## Minimal Example

```cp
import { mgdStrEq } from "merge-descriptors";

pub fn main() {
    let result = mgdStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
