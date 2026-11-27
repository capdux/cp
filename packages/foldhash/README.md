[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# foldhash

Zero-allocation 64x64->128-bit folded-multiply non-cryptographic hasher with Fast and Quality modes (Rust foldhash parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:foldhash` ([https://github.com/orlp/foldhash](https://github.com/orlp/foldhash))

| Metadata | Value |
| --- | --- |
| Package | `foldhash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add foldhash
```

```cp
import {
    FoldHasher,
    makeU64,
    addWrap64,
    foldedMultiply,
    byteShift64,
    initFoldSeed,
    initAccum,
    hashBytesFast,
} from "foldhash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FoldHasher` | `class` | Fields: `accum: u64, sponge: u64, sponge_len: u8, fold_seed: u64, quality: bool, total_len: u64` · Methods: `initFast, initQuality, writeU8, writeU32, writeU64, writeBytes, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | Invokes `makeU64` and returns `u64`. |
| `addWrap64` | `(a: u64, b: u64): u64` | Mutates or appends state via `addWrap64`. |
| `foldedMultiply` | `(x: u64, y: u64): u64` | Invokes `foldedMultiply` and returns `u64`. |
| `byteShift64` | `(idx: u8): u64` | Invokes `byteShift64` and returns `u64`. |
| `initFoldSeed` | `(seed: u64): u64` | Constructs/initializes an instance via `initFoldSeed`. |
| `initAccum` | `(seed: u64): u64` | Constructs/initializes an instance via `initAccum`. |
| `hashBytesFast` | `(data: &[]u8, seed: u64): u64` | Checks predicate `hashBytesFast` and returns `bool`. |
| `hashBytesQuality` | `(data: &[]u8, seed: u64): u64` | Checks predicate `hashBytesQuality` and returns `bool`. |
| `hashU64` | `(v: u64, seed: u64): u64` | Checks predicate `hashU64` and returns `bool`. |

## Minimal Example

```cp
import { hashBytesFast } from "foldhash";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hashBytesFast(out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
