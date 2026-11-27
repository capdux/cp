[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ecdsa

Pure CP zero-allocation ECDSA signature codec (RFC 3279 ASN.1 DER <-> fixed r||s for P-256/P-384, BIP-62 Low-S normalization, constant-time equality via der-asn1 and subtle-ct)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ecdsa` ([https://github.com/RustCrypto/signatures](https://github.com/RustCrypto/signatures))（原库名 `ecdsa` → CP 包名 `ecdsa`）

| Metadata | Value |
| --- | --- |
| Package | `ecdsa` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ecdsa
```

```cp
import {
    EcdsaSigResult,
    EcdsaSignature,
    ecsigZero48,
    ecsigIsAllZero,
    ecsigP256Order,
    ecsigP256HalfOrder,
    ecsigCmpBe32,
    ecsigWriteBeInt,
} from "ecdsa";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EcdsaSigResult` | `class` | Fields: `ok: bool, sig: EcdsaSignature, err: i64` |
| `EcdsaSignature` | `class` | Fields: `r: [48]u8, s: [48]u8, coord_len: usize` · Methods: `empty, fromFixed, fromDer, toFixed, toDer, isLowSP256, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ecsigZero48` | `(): [48]u8` | Invokes `ecsigZero48` and returns `[48]u8`. |
| `ecsigIsAllZero` | `(data: &[]u8): bool` | Invokes `ecsigIsAllZero` and returns `bool`. |
| `ecsigP256Order` | `(): [32]u8` | Invokes `ecsigP256Order` and returns `[32]u8`. |
| `ecsigP256HalfOrder` | `(): [32]u8` | Invokes `ecsigP256HalfOrder` and returns `[32]u8`. |
| `ecsigCmpBe32` | `(a: &[]u8, b: &[]u8): i32` | Invokes `ecsigCmpBe32` and returns `i32`. |
| `ecsigWriteBeInt` | `(w: &mut DerWriter, val: &[]u8): bool` | Invokes `ecsigWriteBeInt` and returns `bool`. |
| `ecsigReadBeInt` | `(r: &mut DerReader, coord_len: usize, out: []u8): i64` | Invokes `ecsigReadBeInt` and returns `i64`. |

## Minimal Example

```cp
import { ecsigZero48 } from "ecdsa";

pub fn main() {
    let result = ecsigZero48();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
