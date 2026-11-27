[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cbc

Zero-allocation pure CP AES-128-CBC and AES-256-CBC block cipher mode with PKCS#7 padding (RustCrypto cbc parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cbc` ([https://github.com/RustCrypto/block-modes](https://github.com/RustCrypto/block-modes))

| Metadata | Value |
| --- | --- |
| Package | `cbc` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cbc
```

```cp
import {
    cbcmReadBlock,
    cbcmWriteBlock,
    cbcmXorBlock,
    cbcmValidatePkcs7,
    aes128CbcEncryptBlocks,
    aes128CbcDecryptBlocks,
    aes128CbcEncryptPkcs7,
    aes128CbcDecryptPkcs7,
} from "cbc";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cbcmReadBlock` | `(src: &[]u8, offset: usize): [16]u8` | Invokes `cbcmReadBlock` and returns `[16]u8`. |
| `cbcmWriteBlock` | `(dst: []u8, offset: usize, blk: [16]u8)` | Executes `cbcmWriteBlock` in-place. |
| `cbcmXorBlock` | `(a: [16]u8, b: [16]u8): [16]u8` | Invokes `cbcmXorBlock` and returns `[16]u8`. |
| `cbcmValidatePkcs7` | `(lastBlock: [16]u8): i64` | Invokes `cbcmValidatePkcs7` and returns `i64`. |
| `aes128CbcEncryptBlocks` | `(key: [16]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize` | Invokes `aes128CbcEncryptBlocks` and returns `usize`. |
| `aes128CbcDecryptBlocks` | `(key: [16]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): usize` | Invokes `aes128CbcDecryptBlocks` and returns `usize`. |
| `aes128CbcEncryptPkcs7` | `(key: [16]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize` | Invokes `aes128CbcEncryptPkcs7` and returns `usize`. |
| `aes128CbcDecryptPkcs7` | `(key: [16]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): i64` | Invokes `aes128CbcDecryptPkcs7` and returns `i64`. |
| `aes256CbcEncryptBlocks` | `(key: [32]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize` | Invokes `aes256CbcEncryptBlocks` and returns `usize`. |
| `aes256CbcDecryptBlocks` | `(key: [32]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): usize` | Invokes `aes256CbcDecryptBlocks` and returns `usize`. |
| `aes256CbcEncryptPkcs7` | `(key: [32]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize` | Invokes `aes256CbcEncryptPkcs7` and returns `usize`. |
| `aes256CbcDecryptPkcs7` | `(key: [32]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): i64` | Invokes `aes256CbcDecryptPkcs7` and returns `i64`. |

## Minimal Example

```cp
import { cbcmReadBlock } from "cbc";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = cbcmReadBlock(out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
