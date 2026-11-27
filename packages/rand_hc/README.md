[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rand_hc

Zero-allocation HC-128 eSTREAM cryptographic stream cipher PRNG (Hongjun Wu specification) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_hc` ([https://github.com/rust-random/rand](https://github.com/rust-random/rand), `MIT OR Apache-2.0`)

| Metadata | Value |
| --- | --- |
| Package | `rand_hc` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rand_hc
```

```cp
import {
    Hc128Rng,
    rhcRotr32,
    rhcRotl32,
    rhcF1,
    rhcF2,
    rhcG1,
    rhcG2,
    rhcReadU32Le,
} from "rand_hc";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Hc128Rng` | `class` | Fields: `p: [512]u32, q: [512]u32, counter: usize` · Methods: `h1, h2, fromSeed, nextU32, nextU64, fillBytes` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rhcRotr32` | `(x: u32, n: usize): u32` | Invokes `rhcRotr32` and returns `u32`. |
| `rhcRotl32` | `(x: u32, n: usize): u32` | Invokes `rhcRotl32` and returns `u32`. |
| `rhcF1` | `(x: u32): u32` | Invokes `rhcF1` and returns `u32`. |
| `rhcF2` | `(x: u32): u32` | Invokes `rhcF2` and returns `u32`. |
| `rhcG1` | `(x: u32, y: u32, z: u32): u32` | Invokes `rhcG1` and returns `u32`. |
| `rhcG2` | `(x: u32, y: u32, z: u32): u32` | Invokes `rhcG2` and returns `u32`. |
| `rhcReadU32Le` | `(seed: [32]u8, offset: usize): u32` | Invokes `rhcReadU32Le` and returns `u32`. |

## Minimal Example

```cp
import { rhcRotr32 } from "rand_hc";

pub fn main() {
    let result = rhcRotr32(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
