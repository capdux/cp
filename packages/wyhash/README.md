[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# wyhash

Zero-allocation Wang Yi wyhash v4 64-bit hash and WyRng PRNG with overflow-free 128-bit multiply in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wyhash` ([https://github.com/eldruin/wyhash-rs](https://github.com/eldruin/wyhash-rs))

| Metadata | Value |
| --- | --- |
| Package | `wyhash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add wyhash
```

```cp
import {
    Mul128,
    WyRng,
    makeU64,
    mul128,
    wymum,
    read4,
    read8,
    wyhashBytes,
} from "wyhash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Mul128` | `class` | Fields: `lo: u64, hi: u64` |
| `WyRng` | `class` | Fields: `state: u64` · Methods: `new, nextU64, nextU32, nextF64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | Invokes `makeU64` and returns `u64`. |
| `mul128` | `(a: u64, b: u64): Mul128` | Invokes `mul128` and returns `Mul128`. |
| `wymum` | `(a: u64, b: u64): u64` | Invokes `wymum` and returns `u64`. |
| `read4` | `(data: &[]u8, off: usize): u64` | Invokes `read4` and returns `u64`. |
| `read8` | `(data: &[]u8, off: usize): u64` | Invokes `read8` and returns `u64`. |
| `wyhashBytes` | `(data: &[]u8, seed: u64): u64` | Invokes `wyhashBytes` and returns `u64`. |
| `wyhash` | `(data: string, seed: u64): u64` | Invokes `wyhash` and returns `u64`. |

## Minimal Example

```cp
import { wyhash } from "wyhash";

pub fn main() {
    let result = wyhash("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
