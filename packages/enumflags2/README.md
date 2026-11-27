[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# enumflags2

Zero-allocation 64-bit type-safe enum bitflags set with valid-mask protection for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:enumflags2` ([https://github.com/meithecatte/enumflags2](https://github.com/meithecatte/enumflags2))

| Metadata | Value |
| --- | --- |
| Package | `enumflags2` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add enumflags2
```

```cp
import { BitFlags64, ef2Popcount64, ef2Ctz64 } from "enumflags2";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BitFlags64` | `class` | Fields: `bits: u64, valid_mask: u64` · Methods: `empty, all, fromBitsTruncate, fromBitsStrict, raw, len, +15 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ef2Popcount64` | `(val: u64): usize` | Invokes `ef2Popcount64` and returns `usize`. |
| `ef2Ctz64` | `(val: u64): u8` | Invokes `ef2Ctz64` and returns `u8`. |

## Minimal Example

```cp
import { ef2Popcount64 } from "enumflags2";

pub fn main() {
    let result = ef2Popcount64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
