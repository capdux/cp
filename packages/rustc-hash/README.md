[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rustc-hash

Zero-heap pure-CP fast non-cryptographic FxHasher (64-bit & 32-bit, crates.io rustc-hash equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustc-hash` ([https://github.com/rust-lang/rustc-hash](https://github.com/rust-lang/rustc-hash))

| Metadata | Value |
| --- | --- |
| Package | `rustc-hash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rustc-hash
```

```cp
import {
    FxHasher64,
    FxHasher32,
    fxhMul64,
    fxhMul32,
    fxhRotl64By5,
    fxhRotl32By5,
    fxhAddWord64,
    fxhAddWord32,
} from "rustc-hash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FxHasher64` | `class` | Fields: `hash: u64` · Methods: `new, withSeed, writeUsize, writeU64, writeU32, writeBytes, +3 more` |
| `FxHasher32` | `class` | Fields: `hash: u32` · Methods: `new, withSeed, writeU32, writeBytes, writeStr, finish, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fxhMul64` | `(a: u64, b: u64): u64` | Invokes `fxhMul64` and returns `u64`. |
| `fxhMul32` | `(a: u32, b: u32): u32` | Invokes `fxhMul32` and returns `u32`. |
| `fxhRotl64By5` | `(x: u64): u64` | Invokes `fxhRotl64By5` and returns `u64`. |
| `fxhRotl32By5` | `(x: u32): u32` | Invokes `fxhRotl32By5` and returns `u32`. |
| `fxhAddWord64` | `(hash: u64, word: u64): u64` | Invokes `fxhAddWord64` and returns `u64`. |
| `fxhAddWord32` | `(hash: u32, word: u32): u32` | Invokes `fxhAddWord32` and returns `u32`. |
| `fxhHashSlice64` | `(hash: u64, data: &[]u8): u64` | Invokes `fxhHashSlice64` and returns `u64`. |
| `fxhHashSlice32` | `(hash: u32, data: &[]u8): u32` | Invokes `fxhHashSlice32` and returns `u32`. |
| `fxHash64` | `(data: []u8): u64` | Invokes `fxHash64` and returns `u64`. |
| `fxHash64Str` | `(s: string): u64` | Invokes `fxHash64Str` and returns `u64`. |
| `fxHash32` | `(data: []u8): u32` | Invokes `fxHash32` and returns `u32`. |

## Minimal Example

```cp
import { fxHash64Str } from "rustc-hash";

pub fn main() {
    let result = fxHash64Str("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
