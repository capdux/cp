[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fastbloom

Zero-allocation Kirsch-Mitzenmacher double-hashing Bloom filters BloomFilter1024/BloomFilter4096 (Rust fastbloom / bloomfilter parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fastbloom` ([https://github.com/tomtomwombat/fastbloom](https://github.com/tomtomwombat/fastbloom))（原库名 `fastbloom` → CP 包名 `fastbloom`）

| Metadata | Value |
| --- | --- |
| Package | `fastbloom` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fastbloom
```

```cp
import {
    HashPair,
    BloomFilter1024,
    BloomFilter4096,
    makeU64,
    addWrap64,
    mulWrap64,
    splitMix64,
    hashBytesPair,
} from "fastbloom";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HashPair` | `class` | Fields: `h1: u64, h2: u64` |
| `BloomFilter1024` | `class` | Fields: `words: [16]u64, num_hashes: u8, items: usize` · Methods: `init, insert, mightContain, insertU64, mightContainU64, bitCount, +5 more` |
| `BloomFilter4096` | `class` | Fields: `words: [64]u64, num_hashes: u8, items: usize` · Methods: `init, insert, mightContain, insertU64, mightContainU64, bitCount, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | Invokes `makeU64` and returns `u64`. |
| `addWrap64` | `(a: u64, b: u64): u64` | Mutates or appends state via `addWrap64`. |
| `mulWrap64` | `(a: u64, b: u64): u64` | Invokes `mulWrap64` and returns `u64`. |
| `splitMix64` | `(x_in: u64): u64` | Invokes `splitMix64` and returns `u64`. |
| `hashBytesPair` | `(s: string): HashPair` | Checks predicate `hashBytesPair` and returns `bool`. |
| `hashU64Pair` | `(val: u64): HashPair` | Checks predicate `hashU64Pair` and returns `bool`. |
| `bitMask64` | `(bit: usize): u64` | Invokes `bitMask64` and returns `u64`. |
| `popcount64` | `(x_in: u64): usize` | Invokes `popcount64` and returns `usize`. |
| `clampHashes` | `(k: u8): u8` | Invokes `clampHashes` and returns `u8`. |
| `expNeg` | `(x_in: f64): f64` | Invokes `expNeg` and returns `f64`. |
| `computeFpr` | `(k: u8, n: usize, m: usize): f64` | Invokes `computeFpr` and returns `f64`. |

## Minimal Example

```cp
import { hashBytesPair } from "fastbloom";

pub fn main() {
    let result = hashBytesPair("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
