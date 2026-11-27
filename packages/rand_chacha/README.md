[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rand_chacha

Zero-allocation RFC 8439 ChaCha8 / ChaCha12 / ChaCha20 block function and CSPRNG for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_chacha` ([https://github.com/rust-random/rand](https://github.com/rust-random/rand)) / `crates.io:ppv-lite86` ([https://github.com/cryptocorrosion/cryptocorrosion](https://github.com/cryptocorrosion/cryptocorrosion))（原库名 `rand_chacha` → CP 包名 `rand_chacha`）

| Metadata | Value |
| --- | --- |
| Package | `rand_chacha` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rand_chacha
```

```cp
import {
    ChaCha20Rng,
    crAdd32,
    crRotl32,
    crReadLe32,
    crWriteLe32,
    crQuarterRound,
    chachaBlock,
} from "rand_chacha";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ChaCha20Rng` | `class` | Fields: `key: [8]u32, nonce: [3]u32, counter: u32, buf: [64]u8, pos: usize` · Methods: `fromSeed, fromU64, setWordPos, fillBytes, nextU32, nextU64, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `crAdd32` | `(a: u32, b: u32): u32` | Invokes `crAdd32` and returns `u32`. |
| `crRotl32` | `(x: u32, bits: u32): u32` | Invokes `crRotl32` and returns `u32`. |
| `crReadLe32` | `(data: &[]u8, offset: usize): u32` | Invokes `crReadLe32` and returns `u32`. |
| `crWriteLe32` | `(out: []u8, offset: usize, v: u32)` | Executes `crWriteLe32` in-place. |
| `crQuarterRound` | `(st: []u32, a: usize, b: usize, c: usize, d: usize)` | Executes `crQuarterRound` in-place. |
| `chachaBlock` | `(key: [8]u32, counter: u32, nonce: [3]u32, rounds: u32, out: []u8)` | Executes `chachaBlock` in-place. |

## Minimal Example

```cp
import { crAdd32 } from "rand_chacha";

pub fn main() {
    let result = crAdd32(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
