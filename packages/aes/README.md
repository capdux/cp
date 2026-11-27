[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# aes

Zero-allocation pure CP FIPS-197 AES-128, AES-192, and AES-256 block cipher (RustCrypto aes parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:aes` ([https://github.com/RustCrypto/block-ciphers](https://github.com/RustCrypto/block-ciphers))

| Metadata | Value |
| --- | --- |
| Package | `aes` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add aes
```

```cp
import {
    Aes128,
    Aes192,
    Aes256,
    aesXtime,
    aesMul,
    aesSubBytes,
    aesInvSubBytes,
    aesShiftRows,
} from "aes";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Aes128` | `class` | Fields: `rk: [176]u8` · Methods: `new, encryptBlock, decryptBlock` |
| `Aes192` | `class` | Fields: `rk: [208]u8` · Methods: `new, encryptBlock, decryptBlock` |
| `Aes256` | `class` | Fields: `rk: [240]u8` · Methods: `new, encryptBlock, decryptBlock` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `aesXtime` | `(x: u8): u8` | Invokes `aesXtime` and returns `u8`. |
| `aesMul` | `(a_in: u8, b_in: u8): u8` | Invokes `aesMul` and returns `u8`. |
| `aesSubBytes` | `(state: []u8)` | Executes `aesSubBytes` in-place. |
| `aesInvSubBytes` | `(state: []u8)` | Executes `aesInvSubBytes` in-place. |
| `aesShiftRows` | `(s: []u8)` | Executes `aesShiftRows` in-place. |
| `aesInvShiftRows` | `(s: []u8)` | Executes `aesInvShiftRows` in-place. |
| `aesMixColumns` | `(s: []u8)` | Executes `aesMixColumns` in-place. |
| `aesInvMixColumns` | `(s: []u8)` | Executes `aesInvMixColumns` in-place. |
| `aesAddRoundKey` | `(s: []u8, rk: &[]u8, round: usize)` | Executes `aesAddRoundKey` in-place. |
| `aesExpandKey` | `(key: &[]u8, nk: usize, nr: usize, rk: []u8)` | Executes `aesExpandKey` in-place. |
| `aesEncryptCore` | `(block: [16]u8, rk: &[]u8, nr: usize): [16]u8` | Invokes `aesEncryptCore` and returns `[16]u8`. |
| `aesDecryptCore` | `(block: [16]u8, rk: &[]u8, nr: usize): [16]u8` | Invokes `aesDecryptCore` and returns `[16]u8`. |

## Minimal Example

```cp
import { aesXtime } from "aes";

pub fn main() {
    let result = aesXtime(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
