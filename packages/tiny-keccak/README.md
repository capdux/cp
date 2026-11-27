[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tiny-keccak

Zero-allocation Keccak-f[1600] sponge with SHA3-256 (FIPS 202) and Keccak-256 (Ethereum) (Rust tiny-keccak parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tiny-keccak` ([https://github.com/debris/tiny-keccak](https://github.com/debris/tiny-keccak))

| Metadata | Value |
| --- | --- |
| Package | `tiny-keccak` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tiny-keccak
```

```cp
import {
    Sha3_256,
    Keccak256,
    kckU64,
    kckRotl64,
    kckReadLe64,
    kckWriteLe64,
    kckHexDigit,
    kckRoundConst,
} from "tiny-keccak";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Sha3_256` | `class` | Fields: `st: [25]u64, buf: [136]u8, buf_len: usize` · Methods: `new, update, updateStr, finalize, finalizeHex` |
| `Keccak256` | `class` | Fields: `st: [25]u64, buf: [136]u8, buf_len: usize` · Methods: `new, update, updateStr, finalize, finalizeHex` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `kckU64` | `(hi: u32, lo: u32): u64` | Invokes `kckU64` and returns `u64`. |
| `kckRotl64` | `(x: u64, n: u32): u64` | Invokes `kckRotl64` and returns `u64`. |
| `kckReadLe64` | `(b: []u8, off: usize): u64` | Invokes `kckReadLe64` and returns `u64`. |
| `kckWriteLe64` | `(out: []u8, off: usize, v: u64)` | Executes `kckWriteLe64` in-place. |
| `kckHexDigit` | `(nibble: u8): u8` | Invokes `kckHexDigit` and returns `u8`. |
| `kckRoundConst` | `(round: usize): u64` | Invokes `kckRoundConst` and returns `u64`. |
| `kckPermute` | `(st: []u64)` | Executes `kckPermute` in-place. |
| `kckAbsorbBlock` | `(st: []u64, block: []u8)` | Executes `kckAbsorbBlock` in-place. |
| `kckFinalizeSponge` | `(st: []u64, buf: []u8, buf_len: usize, domain_sep: u8, out32: []u8)` | Executes `kckFinalizeSponge` in-place. |
| `kckEncodeHex32` | `(digest: []u8, out64: []u8): usize` | Invokes `kckEncodeHex32` and returns `usize`. |
| `sha3_256Hex` | `(s: string, out64: []u8): usize` | Invokes `sha3_256Hex` and returns `usize`. |
| `keccak256Hex` | `(s: string, out64: []u8): usize` | Invokes `keccak256Hex` and returns `usize`. |

## Minimal Example

```cp
import { sha3_256Hex } from "tiny-keccak";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sha3_256Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
