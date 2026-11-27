[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# vec_map

Zero-allocation O(1) direct-indexed sparse integer-key map (0..127) with 128-bit presence bitmap in CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:vec_map` ([https://github.com/contain-rs/vec-map](https://github.com/contain-rs/vec-map), `MIT OR Apache-2.0`)

| Metadata | Value |
| --- | --- |
| Package | `vec_map` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add vec_map
```

```cp
import { VecMapI64, vmapMask, vmapHasBit } from "vec_map";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `VecMapI64` | `class` | Fields: `slots: [128]i64, bits_lo: u64, bits_hi: u64, count: usize` · Methods: `new, len, isEmpty, containsKey, get, insert, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `vmapMask` | `(bit: usize): u64` | Invokes `vmapMask` and returns `u64`. |
| `vmapHasBit` | `(bits_lo: u64, bits_hi: u64, k: usize): bool` | Invokes `vmapHasBit` and returns `bool`. |

## Minimal Example

```cp
import { vmapMask } from "vec_map";

pub fn main() {
    let result = vmapMask(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
