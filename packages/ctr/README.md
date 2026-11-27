[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ctr

Zero-allocation NIST SP 800-38A AES-CTR stream cipher modes (Ctr128BE / Ctr128LE) with O(1) seek in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ctr` ([https://github.com/RustCrypto/block-modes](https://github.com/RustCrypto/block-modes)) — `MIT OR Apache-2.0`

| Metadata | Value |
| --- | --- |
| Package | `ctr` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ctr
```

```cp
import {
    Aes128Ctr128Be,
    Aes128Ctr128Le,
    Aes256Ctr128Be,
    Aes256Ctr128Le,
    ctrZeroBlock,
    ctrAddBe128,
    ctrAddLe128,
} from "ctr";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Aes128Ctr128Be` | `class` | Fields: `cipher: Aes128, iv: [16]u8, block_idx: u64, buf: [16]u8, buf_pos: usize` · Methods: `new, currentPos, seek, applyKeystream` |
| `Aes128Ctr128Le` | `class` | Fields: `cipher: Aes128, iv: [16]u8, block_idx: u64, buf: [16]u8, buf_pos: usize` · Methods: `new, currentPos, seek, applyKeystream` |
| `Aes256Ctr128Be` | `class` | Fields: `cipher: Aes256, iv: [16]u8, block_idx: u64, buf: [16]u8, buf_pos: usize` · Methods: `new, currentPos, seek, applyKeystream` |
| `Aes256Ctr128Le` | `class` | Fields: `cipher: Aes256, iv: [16]u8, block_idx: u64, buf: [16]u8, buf_pos: usize` · Methods: `new, currentPos, seek, applyKeystream` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ctrZeroBlock` | `(): [16]u8` | Invokes `ctrZeroBlock` and returns `[16]u8`. |
| `ctrAddBe128` | `(iv: [16]u8, delta: u64): [16]u8` | Invokes `ctrAddBe128` and returns `[16]u8`. |
| `ctrAddLe128` | `(iv: [16]u8, delta: u64): [16]u8` | Invokes `ctrAddLe128` and returns `[16]u8`. |

## Minimal Example

```cp
import { ctrZeroBlock } from "ctr";

pub fn main() {
    let result = ctrZeroBlock();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
