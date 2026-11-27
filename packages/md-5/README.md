[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# md-5

Zero-allocation RFC 1321 MD5 cryptographic hash streaming implementation (Rust md-5/md5 crate parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:md-5` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `md-5` → CP 包名 `md-5`）

| Metadata | Value |
| --- | --- |
| Package | `md-5` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add md-5
```

```cp
import {
    Md5Hasher,
    md5Add32,
    md5Add4,
    md5Rotl32,
    md5ShiftAt,
    md5ConstAt,
    md5ReadLe32,
    md5WriteLe32,
} from "md-5";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Md5Hasher` | `class` | Fields: `a0: u32, b0: u32, c0: u32, d0: u32, buf: [64]u8, buf_len: usize, +1 more` · Methods: `new, reset, compressBuf, update, updateStr, finalize, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `md5Add32` | `(a: u32, b: u32): u32` | Invokes `md5Add32` and returns `u32`. |
| `md5Add4` | `(a: u32, b: u32, c: u32, d: u32): u32` | Invokes `md5Add4` and returns `u32`. |
| `md5Rotl32` | `(x: u32, bits: u32): u32` | Invokes `md5Rotl32` and returns `u32`. |
| `md5ShiftAt` | `(idx: usize): u32` | Invokes `md5ShiftAt` and returns `u32`. |
| `md5ConstAt` | `(idx: usize): u32` | Invokes `md5ConstAt` and returns `u32`. |
| `md5ReadLe32` | `(b: []u8, off: usize): u32` | Invokes `md5ReadLe32` and returns `u32`. |
| `md5WriteLe32` | `(out: []u8, off: usize, v: u32)` | Executes `md5WriteLe32` in-place. |
| `md5HexDigit` | `(nibble: u8): u8` | Invokes `md5HexDigit` and returns `u8`. |
| `md5Bytes` | `(data: []u8, out16: []u8)` | Executes `md5Bytes` in-place. |
| `md5Hex` | `(s: string, out32: []u8): usize` | Invokes `md5Hex` and returns `usize`. |

## Minimal Example

```cp
import { md5Hex } from "md-5";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = md5Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
