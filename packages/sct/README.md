[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sct

Zero-allocation RFC 6962 Certificate Transparency Signed Certificate Timestamp (SCT) parser and encoder (rustls/sct.rs parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sct` ([https://github.com/rustls/sct.rs](https://github.com/rustls/sct.rs))

| Metadata | Value |
| --- | --- |
| Package | `sct` |
| Version | `1.0.0` |
| License | `Apache-2.0 OR MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sct
```

```cp
import {
    SctVersion,
    HashAlgorithm,
    SignatureAlgorithm,
    SignedCertificateTimestamp,
    sctInvalid,
    sctReadBeU16,
    sctReadBeU64,
    sctWriteBeU16,
} from "sct";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SctVersion` | `V1, Unknown` |
| `enum` | `HashAlgorithm` | `None, Md5, Sha1, Sha224, Sha256, Sha384, Sha512, Unknown` |
| `enum` | `SignatureAlgorithm` | `Anonymous, Rsa, Dsa, Ecdsa, Ed25519, Ed448, Unknown` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SignedCertificateTimestamp` | `class` | Fields: `valid: bool, version: SctVersion, log_id: [32]u8, timestamp_ms: u64, extensions: &[]u8, hash_alg: HashAlgorithm, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sctInvalid` | `(): SignedCertificateTimestamp` | Invokes `sctInvalid` and returns `SignedCertificateTimestamp`. |
| `sctReadBeU16` | `(b: &[]u8, off: usize): u16` | Invokes `sctReadBeU16` and returns `u16`. |
| `sctReadBeU64` | `(b: &[]u8, off: usize): u64` | Invokes `sctReadBeU64` and returns `u64`. |
| `sctWriteBeU16` | `(v: u16, out: []u8, off: usize)` | Executes `sctWriteBeU16` in-place. |
| `sctWriteBeU64` | `(v: u64, out: []u8, off: usize)` | Executes `sctWriteBeU64` in-place. |
| `sctDecodeHashAlg` | `(b: u8): HashAlgorithm` | Invokes `sctDecodeHashAlg` and returns `HashAlgorithm`. |
| `sctEncodeHashAlg` | `(alg: HashAlgorithm): u8` | Invokes `sctEncodeHashAlg` and returns `u8`. |
| `sctDecodeSigAlg` | `(b: u8): SignatureAlgorithm` | Invokes `sctDecodeSigAlg` and returns `SignatureAlgorithm`. |
| `sctEncodeSigAlg` | `(alg: SignatureAlgorithm): u8` | Invokes `sctEncodeSigAlg` and returns `u8`. |
| `parseSct` | `(data: &[]u8): SignedCertificateTimestamp` | Parses input via `parseSct` and returns `SignedCertificateTimestamp`. |
| `parseSctList` | `(tls_ext: &[]u8, out: []SignedCertificateTimestamp): i64` | Parses input via `parseSctList` and returns `i64`. |
| `encodeSct` | `(sct: SignedCertificateTimestamp, out: []u8): usize` | Serializes/formats data via `encodeSct` returning `usize`. |

## Minimal Example

```cp
import { parseSct } from "sct";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseSct(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
