[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# phf_generator

Zero-allocation CHD Displace-Hash-Compress perfect hash table builder and O(1) lookup for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:phf_generator` ([https://github.com/rust-phf/rust-phf](https://github.com/rust-phf/rust-phf))（原库名 `phf_generator` → CP 包名 `phf_generator`）

| Metadata | Value |
| --- | --- |
| Package | `phf_generator` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add phf_generator
```

```cp
import {
    PhfHashes,
    PhfBuildResult,
    add64,
    mul64,
    mix64,
    hashKey,
    displace,
    buildPhfWithSeed,
} from "phf_generator";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PhfHashes` | `class` | Fields: `g: u32, f1: u32, f2: u32` |
| `PhfBuildResult` | `class` | Fields: `ok: bool, seed: u64, table_len: usize, disp_len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `add64` | `(a: u64, b: u64): u64` | Mutates or appends state via `add64`. |
| `mul64` | `(a: u64, b: u64): u64` | Invokes `mul64` and returns `u64`. |
| `mix64` | `(z_in: u64): u64` | Invokes `mix64` and returns `u64`. |
| `hashKey` | `(key: string, seed: u64): PhfHashes` | Checks predicate `hashKey` and returns `bool`. |
| `displace` | `(f1: u32, f2: u32, d1: u32, d2: u32, len: usize): usize` | Invokes `displace` and returns `usize`. |
| `buildPhfWithSeed` | `(keys: &[]string, start_seed: u64, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult` | Invokes `buildPhfWithSeed` and returns `PhfBuildResult`. |
| `buildPhf` | `(keys: &[]string, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult` | Invokes `buildPhf` and returns `PhfBuildResult`. |
| `lookupIndex` | `(key: string, seed: u64, keys: &[]string, disps_d1: &[]u32, disps_d2: &[]u32, slot_to_key: &[]i32): i64` | Queries/retrieves data via `lookupIndex` returning `i64`. |

## Minimal Example

```cp
import { hashKey } from "phf_generator";

pub fn main() {
    let result = hashKey("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
