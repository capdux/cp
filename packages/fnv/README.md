[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fnv

Zero-allocation FNV-1a 32/64-bit and Rust rustc-hash FxHash 64-bit streaming non-cryptographic hashers for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustc-hash` ([https://github.com/rust-lang/rustc-hash](https://github.com/rust-lang/rustc-hash)) / `crates.io:fnv` ([https://github.com/servo/rust-fnv](https://github.com/servo/rust-fnv))（原库名 `rustc-hash` → CP 包名 `fnv`）

| Metadata | Value |
| --- | --- |
| Package | `fnv` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fnv
```

```cp
import {
    Fnv32Hasher,
    Fnv64Hasher,
    FxHasher,
    mulWrap32,
    mulWrap64,
    rotl64By5,
    fnv1a32,
    fnv1a64,
} from "fnv";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Fnv32Hasher` | `class` | Fields: `state: u32` · Methods: `init, writeByte, write, finish` |
| `Fnv64Hasher` | `class` | Fields: `state: u64` · Methods: `init, writeByte, write, finish` |
| `FxHasher` | `class` | Fields: `state: u64` · Methods: `init, withSeed, writeU64, writeBytes, finish` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mulWrap32` | `(a: u32, b: u32): u32` | Invokes `mulWrap32` and returns `u32`. |
| `mulWrap64` | `(a: u64, b: u64): u64` | Invokes `mulWrap64` and returns `u64`. |
| `rotl64By5` | `(x: u64): u64` | Invokes `rotl64By5` and returns `u64`. |
| `fnv1a32` | `(data: string): u32` | Invokes `fnv1a32` and returns `u32`. |
| `fnv1a64` | `(data: string): u64` | Invokes `fnv1a64` and returns `u64`. |
| `fxHash64` | `(data: string): u64` | Invokes `fxHash64` and returns `u64`. |

## Minimal Example

```cp
import { fnv1a32 } from "fnv";

pub fn main() {
    let result = fnv1a32("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
