[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# thin-vec

Zero-heap compact ThinVec<i64> with push/pop/insert/remove/swapRemove/dedup/truncate and packed metadata header (crates.io thin-vec equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:thin-vec` ([https://github.com/gankra/thin-vec](https://github.com/gankra/thin-vec))

| Metadata | Value |
| --- | --- |
| Package | `thin-vec` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add thin-vec
```

```cp
import { ThinVecI64 } from "thin-vec";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ThinVecI64` | `class` | Fields: `len_val: usize, data: [64]i64` · Methods: `new, fromSlice, len, capacity, isEmpty, get, +11 more` |

## Minimal Example

```cp
import { ThinVecI64 } from "thin-vec";

pub fn main() {
    // Refer to src/index.d.cp for ThinVecI64 fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
