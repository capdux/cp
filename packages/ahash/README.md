[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ahash

Zero-allocation randomized high-speed non-cryptographic hasher with 4-key RandomState, folded multiply, and rotation rounds (Rust ahash parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ahash` ([https://github.com/tkaitchuck/aHash](https://github.com/tkaitchuck/aHash))

| Metadata | Value |
| --- | --- |
| Package | `ahash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ahash
```

```cp
import {
    AHasher,
    RandomState,
    makeU64,
    addWrap64,
    mulWrap64,
    pow2_64,
    rotl64,
    foldedMultiply,
} from "ahash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AHasher` | `class` | Fields: `buffer: u64, pad: u64, extra_keys: [2]u64, tail: [8]u8, tail_len: usize, total_len: u64` · Methods: `init, writeU8, writeU32, writeU64, writeBytes, finish` |
| `RandomState` | `class` | Fields: `k0: u64, k1: u64, k2: u64, k3: u64` · Methods: `withSeeds, defaultState, buildHasher, hashOneBytes, hashOneU64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | Invokes `makeU64` and returns `u64`. |
| `addWrap64` | `(a: u64, b: u64): u64` | Mutates or appends state via `addWrap64`. |
| `mulWrap64` | `(a: u64, b: u64): u64` | Invokes `mulWrap64` and returns `u64`. |
| `pow2_64` | `(n: u32): u64` | Invokes `pow2_64` and returns `u64`. |
| `rotl64` | `(x: u64, r_in: u32): u64` | Invokes `rotl64` and returns `u64`. |
| `foldedMultiply` | `(x: u64, y: u64): u64` | Invokes `foldedMultiply` and returns `u64`. |
| `multipleConst` | `(): u64` | Invokes `multipleConst` and returns `u64`. |
| `pi0` | `(): u64` | Invokes `pi0` and returns `u64`. |
| `pi1` | `(): u64` | Invokes `pi1` and returns `u64`. |
| `pi2` | `(): u64` | Invokes `pi2` and returns `u64`. |
| `pi3` | `(): u64` | Invokes `pi3` and returns `u64`. |
| `packTail` | `(tail: [8]u8, tail_len: usize): u64` | Invokes `packTail` and returns `u64`. |
| `hashBytes` | `(data: &[]u8, k0: u64, k1: u64): u64` | Checks predicate `hashBytes` and returns `bool`. |
| `hashU64` | `(v: u64, k0: u64, k1: u64): u64` | Checks predicate `hashU64` and returns `bool`. |

## Minimal Example

```cp
import { hashBytes } from "ahash";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hashBytes(out_buf[0..256], 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
