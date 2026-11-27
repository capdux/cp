[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bitvec

Zero-heap arbitrary-length bit vector with popcount, find-first-one, bitwise ops and shifts (clean-room CP implementation of crates.io bitvec)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bitvec` ([https://github.com/ferrilab/bitvec](https://github.com/ferrilab/bitvec))

| Metadata | Value |
| --- | --- |
| Package | `bitvec` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bitvec
```

```cp
import { BitVec512, bvPopcountU64 } from "bitvec";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BitVec512` | `class` | Fields: `words: [8]u64, bit_len: usize` · Methods: `new, withLen, len, capacity, isEmpty, clear, +16 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bvPopcountU64` | `(x: u64): usize` | Invokes `bvPopcountU64` and returns `usize`. |

## Minimal Example

```cp
import { bvPopcountU64 } from "bitvec";

pub fn main() {
    let result = bvPopcountU64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
