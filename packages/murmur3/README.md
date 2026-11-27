[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# murmur3

Zero-heap MurmurHash3 (32-bit x86 & 128-bit x64) deterministic non-cryptographic hash functions for sharding, Kafka/Cassandra/ClickHouse partitioning

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:murmur3` ([https://github.com/stusmall/murmur3](https://github.com/stusmall/murmur3))

| Metadata | Value |
| --- | --- |
| Package | `murmur3` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add murmur3
```

```cp
import {
    Hash128,
    rotl32,
    rotl64,
    fmix32,
    fmix64,
    murmur3_32,
    murmur3_128,
} from "murmur3";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Hash128` | `class` | Fields: `lo: u64, hi: u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rotl32` | `(x: u32, r: u5): u32` | Invokes `rotl32` and returns `u32`. |
| `rotl64` | `(x: u64, r: u6): u64` | Invokes `rotl64` and returns `u64`. |
| `fmix32` | `(h_in: u32): u32` | Invokes `fmix32` and returns `u32`. |
| `fmix64` | `(k_in: u64): u64` | Invokes `fmix64` and returns `u64`. |
| `murmur3_32` | `(data: string, seed: u32): u32` | Invokes `murmur3_32` and returns `u32`. |
| `murmur3_128` | `(data: string, seed: u64): Hash128` | Invokes `murmur3_128` and returns `Hash128`. |

## Minimal Example

```cp
import { murmur3_32 } from "murmur3";

pub fn main() {
    let result = murmur3_32("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
