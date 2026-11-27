[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bloom-filters

Zero-heap classic Bloom filter in pure CP with Kirsch-Mitzenmacher double hashing

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bloom-filters` ([https://github.com/Callidon/bloom-filters](https://github.com/Callidon/bloom-filters)) / `crates.io:bloomfilter` ([https://github.com/jedisct1/rust-bloom-filter](https://github.com/jedisct1/rust-bloom-filter))（原库名 `bloom-filters` → CP 包名 `bloom-filters`）

| Metadata | Value |
| --- | --- |
| Package | `bloom-filters` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bloom-filters
```

```cp
import {
    BloomFilter,
    fnv1a64,
    djb2Mix64,
    popcount,
    fillRatio,
    optimalNumHashes,
} from "bloom-filters";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BloomFilter` | `class` | Fields: `num_hashes: u32, bit_count: usize, inserted: u64` · Methods: `new, clear, insertBytes, insert, containsBytes, contains, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fnv1a64` | `(data: &[]u8): u64` | Invokes `fnv1a64` and returns `u64`. |
| `djb2Mix64` | `(data: &[]u8): u64` | Invokes `djb2Mix64` and returns `u64`. |
| `popcount` | `(bits: &[]u8): u64` | Invokes `popcount` and returns `u64`. |
| `fillRatio` | `(bits: &[]u8): f64` | Invokes `fillRatio` and returns `f64`. |
| `optimalNumHashes` | `(bit_count: usize, expected_items: usize): u32` | Invokes `optimalNumHashes` and returns `u32`. |

## Minimal Example

```cp
import { fnv1a64 } from "bloom-filters";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = fnv1a64(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
