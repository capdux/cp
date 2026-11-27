[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# blake2

Zero-allocation RFC 7693 BLAKE2s-256 streaming cryptographic hash & keyed MAC (Rust blake2 crate parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:blake2` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `blake2` → CP 包名 `blake2`）

| Metadata | Value |
| --- | --- |
| Package | `blake2` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add blake2
```

```cp
import {
    Blake2s256,
    bl2sAdd2,
    bl2sAdd3,
    bl2sRotr16,
    bl2sRotr12,
    bl2sRotr8,
    bl2sRotr7,
    bl2sReadLe32,
} from "blake2";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Blake2s256` | `class` | Fields: `h: [8]u32, t: u64, buf: [64]u8, buf_len: usize` · Methods: `new, newKeyed, update, updateStr, finalize, finalizeHex` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bl2sAdd2` | `(a: u32, b: u32): u32` | Invokes `bl2sAdd2` and returns `u32`. |
| `bl2sAdd3` | `(a: u32, b: u32, c: u32): u32` | Invokes `bl2sAdd3` and returns `u32`. |
| `bl2sRotr16` | `(x: u32): u32` | Invokes `bl2sRotr16` and returns `u32`. |
| `bl2sRotr12` | `(x: u32): u32` | Invokes `bl2sRotr12` and returns `u32`. |
| `bl2sRotr8` | `(x: u32): u32` | Invokes `bl2sRotr8` and returns `u32`. |
| `bl2sRotr7` | `(x: u32): u32` | Invokes `bl2sRotr7` and returns `u32`. |
| `bl2sReadLe32` | `(b: []u8, off: usize): u32` | Invokes `bl2sReadLe32` and returns `u32`. |
| `bl2sWriteLe32` | `(out: []u8, off: usize, v: u32)` | Executes `bl2sWriteLe32` in-place. |
| `bl2sHexDigit` | `(nibble: u8): u8` | Invokes `bl2sHexDigit` and returns `u8`. |
| `bl2sIv` | `(idx: usize): u32` | Invokes `bl2sIv` and returns `u32`. |
| `bl2sMix` | `(v: []u32, ai: usize, bi: usize, ci: usize, di: usize, x: u32, y: u32)` | Executes `bl2sMix` in-place. |
| `bl2sCompress` | `(h: []u32, block: []u8, t: u64, is_last: bool)` | Executes `bl2sCompress` in-place. |
| `blake2s256Bytes` | `(data: []u8, out32: []u8)` | Executes `blake2s256Bytes` in-place. |
| `blake2s256Hex` | `(s: string, out64: []u8): usize` | Invokes `blake2s256Hex` and returns `usize`. |

## Minimal Example

```cp
import { blake2s256Hex } from "blake2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = blake2s256Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
