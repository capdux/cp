[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# xorf

Zero-allocation Xor8 static fingerprint filter (Graf & Lemire ACM JEA 2020 / Rust xorf parity) with O(1) 3-lookup XOR queries and ~0.39% false-positive rate for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:xorf` ([https://github.com/ayazhafiz/xorf](https://github.com/ayazhafiz/xorf))（原库名 `xorf` → CP 包名 `xorf`）

| Metadata | Value |
| --- | --- |
| Package | `xorf` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add xorf
```

```cp
import {
    Slots3,
    Xor8Filter,
    makeU64,
    hashKey64,
    deriveSlots,
} from "xorf";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Slots3` | `class` | Fields: `h0: usize, h1: usize, h2: usize, fp: u8` |
| `Xor8Filter` | `class` | Fields: `seed: u64, block_length: usize, fingerprints: [MAX_SLOTS]u8, key_count: usize, valid: bool` · Methods: `emptyWithStatus, build, contains, len, fingerprintBytes, isValid` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | Invokes `makeU64` and returns `u64`. |
| `hashKey64` | `(key: u64, seed: u64): u64` | Checks predicate `hashKey64` and returns `bool`. |
| `deriveSlots` | `(h: u64, block_len: usize): Slots3` | Invokes `deriveSlots` and returns `Slots3`. |

## Minimal Example

```cp
import { hashKey64 } from "xorf";

pub fn main() {
    let result = hashKey64(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
