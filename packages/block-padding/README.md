[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# block-padding

Zero-allocation PKCS#7 (constant-time padding-oracle-resistant unpad), ISO 7816-4, ANSI X9.23, ZeroPadding, 128-bit BE/LE CTR counter increment, and XOR block primitives reusing subtle-ct in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:block-padding` ([https://github.com/RustCrypto/utils](https://github.com/RustCrypto/utils))

| Metadata | Value |
| --- | --- |
| Package | `block-padding` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add block-padding
```

```cp
import {
    bpCopyBytes,
    pkcs7Pad,
    pkcs7UnpadCt,
    iso7816Pad,
    iso7816Unpad,
    ansiX923Pad,
    ansiX923Unpad,
    zeroPad,
} from "block-padding";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bpCopyBytes` | `(dst: []u8, src: &[]u8)` | Executes `bpCopyBytes` in-place. |
| `pkcs7Pad` | `(input: &[]u8, block_size: usize, out: []u8): i64` | Invokes `pkcs7Pad` and returns `i64`. |
| `pkcs7UnpadCt` | `(padded: &[]u8, block_size: usize): i64` | Invokes `pkcs7UnpadCt` and returns `i64`. |
| `iso7816Pad` | `(input: &[]u8, block_size: usize, out: []u8): i64` | Checks predicate `iso7816Pad` and returns `bool`. |
| `iso7816Unpad` | `(padded: &[]u8, block_size: usize): i64` | Checks predicate `iso7816Unpad` and returns `bool`. |
| `ansiX923Pad` | `(input: &[]u8, block_size: usize, out: []u8): i64` | Invokes `ansiX923Pad` and returns `i64`. |
| `ansiX923Unpad` | `(padded: &[]u8, block_size: usize): i64` | Invokes `ansiX923Unpad` and returns `i64`. |
| `zeroPad` | `(input: &[]u8, block_size: usize, out: []u8): i64` | Invokes `zeroPad` and returns `i64`. |
| `zeroUnpad` | `(padded: &[]u8, block_size: usize): i64` | Invokes `zeroUnpad` and returns `i64`. |
| `ctrIncBe128` | `(counter: []u8)` | Executes `ctrIncBe128` in-place. |
| `ctrIncLe128` | `(counter: []u8)` | Executes `ctrIncLe128` in-place. |
| `xorBlock` | `(dst: []u8, a: &[]u8, b: &[]u8): bool` | Invokes `xorBlock` and returns `bool`. |
| `wipeBlock` | `(buf: []u8)` | Executes `wipeBlock` in-place. |

## Minimal Example

```cp
import { iso7816Pad } from "block-padding";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = iso7816Pad(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
