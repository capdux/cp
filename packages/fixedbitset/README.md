[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fixedbitset

Zero-allocation fixed-size bitsets BitSet256/BitSet1024 with SWAR popcount, fast scanning, and set algebra (Rust fixedbitset / bit-set parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`fixedbitset`](https://github.com/petgraph/fixedbitset)（`MIT OR Apache-2.0`）（原库名 `fixedbitset`，CP 包名 `fixedbitset`）。

| Metadata | Value |
| --- | --- |
| Package | `fixedbitset` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fixedbitset
```

```cp
import {
    BitSet256,
    BitSet1024,
    allOnes64,
    bitMask64,
    popcount64,
    ctz64,
} from "fixedbitset";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BitSet256` | `class` | Fields: `words: [4]u64` · Methods: `init, full, len, clear, contains, insert, +14 more` |
| `BitSet1024` | `class` | Fields: `words: [16]u64` · Methods: `init, full, len, clear, contains, insert, +14 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `allOnes64` | `(): u64` | Invokes `allOnes64` and returns `u64`. |
| `bitMask64` | `(bit: usize): u64` | Invokes `bitMask64` and returns `u64`. |
| `popcount64` | `(x_in: u64): usize` | Invokes `popcount64` and returns `usize`. |
| `ctz64` | `(w_in: u64): usize` | Invokes `ctz64` and returns `usize`. |

## Minimal Example

```cp
import { allOnes64 } from "fixedbitset";

pub fn main() {
    let result = allOnes64();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
