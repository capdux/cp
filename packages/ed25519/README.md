[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ed25519

Zero-heap RFC 8032 Ed25519 signature & key containers, canonical scalar S < L validation, RFC 8410 SPKI/PKCS#8 DER and hex codecs in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ed25519` ([https://github.com/RustCrypto/signatures](https://github.com/RustCrypto/signatures)) — `Apache-2.0 OR MIT`

| Metadata | Value |
| --- | --- |
| Package | `ed25519` |
| Version | `1.0.0` |
| License | `Apache-2.0 OR MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ed25519
```

```cp
import {
    Ed25519Signature,
    Ed25519PublicKey,
    Ed25519SecretKey,
    ed25Zero32,
    ed25Zero64,
    ed25GroupOrderL,
    ed25SpkiPrefix,
    ed25Pkcs8Prefix,
} from "ed25519";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Ed25519Signature` | `class` | Fields: `bytes: [64]u8` · Methods: `fromBytes, fromSlice, fromComponents, rBytes, sBytes, toBytes, +4 more` |
| `Ed25519PublicKey` | `class` | Fields: `bytes: [32]u8` · Methods: `fromBytes, fromSlice, toBytes, toSpkiDer, fromSpkiDer, fromHex, +2 more` |
| `Ed25519SecretKey` | `class` | Fields: `bytes: [32]u8` · Methods: `fromBytes, fromSlice, toBytes, toPkcs8Der, fromPkcs8Der, fromHex, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ed25Zero32` | `(): [32]u8` | Invokes `ed25Zero32` and returns `[32]u8`. |
| `ed25Zero64` | `(): [64]u8` | Invokes `ed25Zero64` and returns `[64]u8`. |
| `ed25GroupOrderL` | `(): [32]u8` | Invokes `ed25GroupOrderL` and returns `[32]u8`. |
| `ed25SpkiPrefix` | `(): [12]u8` | Invokes `ed25SpkiPrefix` and returns `[12]u8`. |
| `ed25Pkcs8Prefix` | `(): [16]u8` | Invokes `ed25Pkcs8Prefix` and returns `[16]u8`. |
| `ed25HexNibble` | `(ch: u8): i32` | Invokes `ed25HexNibble` and returns `i32`. |
| `ed25HexDigit` | `(n: u8): u8` | Invokes `ed25HexDigit` and returns `u8`. |
| `ed25EncodeHex` | `(src: &[]u8, out: []u8): usize` | Invokes `ed25EncodeHex` and returns `usize`. |

## Minimal Example

```cp
import { ed25Zero32 } from "ed25519";

pub fn main() {
    let result = ed25Zero32();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
