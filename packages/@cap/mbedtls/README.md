[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/mbedtls

ARM Mbed TLS 3.6.2 core cryptography suite (SHA-256/512, AES-GCM, ChaCha20-Poly1305, Base64) for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/mbedtls` |
| Version | `3.6.2` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/mbedtls
```

```cp
import {
    MbedTlsError,
    cp_mbedtls_version,
    cp_mbedtls_sha256,
    cp_mbedtls_aes_gcm_roundtrip_test,
    versionInto,
    sha256,
    aesGcmRoundtripTest,
} from "@cap/mbedtls";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `MbedTlsError` | `BufferTooSmall, HashFailed, AesGcmFailed` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_mbedtls_version` | `(buf: [*]u8!, cap: usize): usize` | Invokes `cp_mbedtls_version` and returns `usize`. |
| `cp_mbedtls_sha256` | `(data: [*]&u8!?, len: usize, out32: [*]u8!): i32` | Invokes `cp_mbedtls_sha256` and returns `i32`. |
| `cp_mbedtls_aes_gcm_roundtrip_test` | `(): i32` | Invokes `cp_mbedtls_aes_gcm_roundtrip_test` and returns `i32`. |
| `versionInto` | `(buf: []u8): !string` | Invokes `versionInto` and returns `!string`. |
| `sha256` | `(data: &[]u8): ![32]u8` | Invokes `sha256` and returns `![32]u8`. |
| `aesGcmRoundtripTest` | `(): bool` | Invokes `aesGcmRoundtripTest` and returns `bool`. |

## Minimal Example

```cp
import { versionInto } from "@cap/mbedtls";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = versionInto(out_buf[0..256]) catch return;
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
