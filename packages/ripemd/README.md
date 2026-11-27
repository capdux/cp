[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ripemd

Zero-heap pure-CP RIPEMD-160 160-bit cryptographic hash function (crates.io ripemd equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ripemd` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `ripemd` → CP 包名 `ripemd`）

| Metadata | Value |
| --- | --- |
| Package | `ripemd` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ripemd
```

```cp
import {
    Ripemd160,
    rmdAdd,
    rmdAdd3,
    rmdAdd4,
    rmdRol,
    rmdNot,
    rmdFunc,
    rmdKl,
} from "ripemd";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Ripemd160` | `class` | Fields: `state: [5]u32, buf: [64]u8, buf_len: usize, total_len: u64` · Methods: `new, reset, update, updateStr, finalize, finalizeHex` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rmdAdd` | `(a: u32, b: u32): u32` | Invokes `rmdAdd` and returns `u32`. |
| `rmdAdd3` | `(a: u32, b: u32, c: u32): u32` | Invokes `rmdAdd3` and returns `u32`. |
| `rmdAdd4` | `(a: u32, b: u32, c: u32, d: u32): u32` | Invokes `rmdAdd4` and returns `u32`. |
| `rmdRol` | `(x: u32, n: u32): u32` | Invokes `rmdRol` and returns `u32`. |
| `rmdNot` | `(x: u32): u32` | Invokes `rmdNot` and returns `u32`. |
| `rmdFunc` | `(round: usize, x: u32, y: u32, z: u32): u32` | Invokes `rmdFunc` and returns `u32`. |
| `rmdKl` | `(round: usize): u32` | Invokes `rmdKl` and returns `u32`. |
| `rmdKr` | `(round: usize): u32` | Invokes `rmdKr` and returns `u32`. |
| `rmdCompress` | `(state: [5]u32, block: []u8): [5]u32` | Invokes `rmdCompress` and returns `[5]u32`. |
| `ripemd160Bytes` | `(data: []u8, out20: []u8)` | Executes `ripemd160Bytes` in-place. |
| `ripemd160Hex` | `(s: string, out40: []u8): usize` | Invokes `ripemd160Hex` and returns `usize`. |

## Minimal Example

```cp
import { ripemd160Hex } from "ripemd";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ripemd160Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
