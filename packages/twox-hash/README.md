[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# twox-hash

Zero-heap pure-CP XXHash32 and XXHash64 fast non-cryptographic streaming hashers (crates.io twox-hash equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:twox-hash` ([https://github.com/shepmaster/twox-hash](https://github.com/shepmaster/twox-hash))

| Metadata | Value |
| --- | --- |
| Package | `twox-hash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add twox-hash
```

```cp
import {
    XxHash32,
    XxHash64,
    xxhU64,
    xxhAdd32,
    xxhSub32,
    xxhMul32,
    xxhRol32,
    xxhAdd64,
} from "twox-hash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `XxHash32` | `class` | Fields: `seed: u32, v1: u32, v2: u32, v3: u32, v4: u32, buf: [16]u8, +2 more` · Methods: `withSeed, oneshot, write, writeStr, finish` |
| `XxHash64` | `class` | Fields: `seed: u64, v1: u64, v2: u64, v3: u64, v4: u64, buf: [32]u8, +2 more` · Methods: `withSeed, oneshot, write, writeStr, finish` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `xxhU64` | `(hi: u32, lo: u32): u64` | Invokes `xxhU64` and returns `u64`. |
| `xxhAdd32` | `(a: u32, b: u32): u32` | Invokes `xxhAdd32` and returns `u32`. |
| `xxhSub32` | `(a: u32, b: u32): u32` | Invokes `xxhSub32` and returns `u32`. |
| `xxhMul32` | `(a: u32, b: u32): u32` | Invokes `xxhMul32` and returns `u32`. |
| `xxhRol32` | `(x: u32, r: u32): u32` | Invokes `xxhRol32` and returns `u32`. |
| `xxhAdd64` | `(a: u64, b: u64): u64` | Invokes `xxhAdd64` and returns `u64`. |
| `xxhSub64` | `(a: u64, b: u64): u64` | Invokes `xxhSub64` and returns `u64`. |
| `xxhMul64` | `(a: u64, b: u64): u64` | Invokes `xxhMul64` and returns `u64`. |
| `xxhRol64` | `(x: u64, r: u64): u64` | Invokes `xxhRol64` and returns `u64`. |
| `xxh32Round` | `(acc: u32, lane: u32): u32` | Invokes `xxh32Round` and returns `u32`. |
| `xxh64Prime1` | `(): u64` | Invokes `xxh64Prime1` and returns `u64`. |
| `xxh64Prime2` | `(): u64` | Invokes `xxh64Prime2` and returns `u64`. |
| `xxh64Prime3` | `(): u64` | Invokes `xxh64Prime3` and returns `u64`. |
| `xxh64Prime4` | `(): u64` | Invokes `xxh64Prime4` and returns `u64`. |
| `xxh64Prime5` | `(): u64` | Invokes `xxh64Prime5` and returns `u64`. |
| `xxh64Round` | `(acc: u64, lane: u64): u64` | Invokes `xxh64Round` and returns `u64`. |
| `xxh64MergeRound` | `(acc: u64, val: u64): u64` | Invokes `xxh64MergeRound` and returns `u64`. |

## Minimal Example

```cp
import { xxhU64 } from "twox-hash";

pub fn main() {
    let result = xxhU64(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
