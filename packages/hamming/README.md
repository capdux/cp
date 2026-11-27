[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hamming

Zero-allocation Hamming distance, byte popcount weight, 64-bit Charikar SimHash, and Hamming(7,4) error-correcting code in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hamming` ([https://github.com/huonw/hamming](https://github.com/huonw/hamming))

| Metadata | Value |
| --- | --- |
| Package | `hamming` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hamming
```

```cp
import {
    Hamming74Decoded,
    popcountU32,
    popcountU8,
    popcountU64,
    hammingU64,
    weightBytes,
    distanceBytes,
    isTokenByte,
} from "hamming";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Hamming74Decoded` | `class` | Fields: `data: u8, corrected: bool, error_bit: u8` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `popcountU32` | `(x: u32): u32` | Invokes `popcountU32` and returns `u32`. |
| `popcountU8` | `(b: u8): u32` | Invokes `popcountU8` and returns `u32`. |
| `popcountU64` | `(x: u64): u32` | Invokes `popcountU64` and returns `u32`. |
| `hammingU64` | `(a: u64, b: u64): u32` | Invokes `hammingU64` and returns `u32`. |
| `weightBytes` | `(data: &[]u8): u64` | Invokes `weightBytes` and returns `u64`. |
| `distanceBytes` | `(a: &[]u8, b: &[]u8): i64` | Invokes `distanceBytes` and returns `i64`. |
| `isTokenByte` | `(b: u8): bool` | Checks predicate `isTokenByte` and returns `bool`. |
| `hashToken64` | `(text: &[]u8, start: usize, end: usize): u64` | Checks predicate `hashToken64` and returns `bool`. |
| `simhash64` | `(text: &[]u8): u64` | Invokes `simhash64` and returns `u64`. |
| `hamming74Encode` | `(nibble: u8): u8` | Invokes `hamming74Encode` and returns `u8`. |
| `hamming74Decode` | `(codeword: u8): Hamming74Decoded` | Invokes `hamming74Decode` and returns `Hamming74Decoded`. |

## Minimal Example

```cp
import { isTokenByte } from "hamming";

pub fn main() {
    let result = isTokenByte(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
