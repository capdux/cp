[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# seahash

Zero-heap pure-CP 4-lane parallel SeaHash 64-bit fast non-cryptographic hash function (crates.io seahash equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:seahash` ([https://github.com/ticki/tfs](https://github.com/ticki/tfs))

| Metadata | Value |
| --- | --- |
| Package | `seahash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add seahash
```

```cp
import {
    SeaHasher,
    seaU64,
    seaMul64,
    seahashDiffuse,
    seahashBytes,
    seahashSeeded,
} from "seahash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SeaHasher` | `class` | Fields: `a: u64, b: u64, c: u64, d: u64, buf: [8]u8, buf_len: usize, +1 more` · Methods: `new, withSeeds, writeU64, writeBytes, writeStr, finish` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `seaU64` | `(hi: u32, lo: u32): u64` | Invokes `seaU64` and returns `u64`. |
| `seaMul64` | `(a: u64, b: u64): u64` | Invokes `seaMul64` and returns `u64`. |
| `seahashDiffuse` | `(x_in: u64): u64` | Invokes `seahashDiffuse` and returns `u64`. |
| `seahashBytes` | `(buf: []u8): u64` | Invokes `seahashBytes` and returns `u64`. |
| `seahashSeeded` | `(buf: []u8, k1: u64, k2: u64, k3: u64, k4: u64): u64` | Invokes `seahashSeeded` and returns `u64`. |

## Minimal Example

```cp
import { seaU64 } from "seahash";

pub fn main() {
    let result = seaU64(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
