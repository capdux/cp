[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rfc6979

Pure CP zero-allocation RFC 6979 deterministic ECDSA nonce generator (HMAC-SHA256 DRBG, RustCrypto rfc6979 parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rfc6979` ([https://github.com/RustCrypto/signatures](https://github.com/RustCrypto/signatures))

| Metadata | Value |
| --- | --- |
| Package | `rfc6979` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rfc6979
```

```cp
import {
    HmacDrbg256,
    rfcCopy32,
    rfcCmpBe32,
    rfcIsNonZero32,
    rfcOrderValid32,
    rfcSubBe32,
    generateK,
} from "rfc6979";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HmacDrbg256` | `class` | Fields: `k: [32]u8, v: [32]u8` · Methods: `init, nextBytes` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rfcCopy32` | `(dst: []u8, src: []u8)` | Executes `rfcCopy32` in-place. |
| `rfcCmpBe32` | `(a: []u8, b: []u8): i32` | Invokes `rfcCmpBe32` and returns `i32`. |
| `rfcIsNonZero32` | `(a: []u8): bool` | Invokes `rfcIsNonZero32` and returns `bool`. |
| `rfcOrderValid32` | `(q: []u8): bool` | Invokes `rfcOrderValid32` and returns `bool`. |
| `rfcSubBe32` | `(a: []u8, b: []u8, out: []u8)` | Executes `rfcSubBe32` in-place. |
| `generateK` | `(priv_key_32: []u8, msg_hash_32: []u8, order_32: []u8, out_k32: []u8): bool` | Invokes `generateK` and returns `bool`. |

## Minimal Example

```cp
import { rfcCmpBe32 } from "rfc6979";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = rfcCmpBe32(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
