[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/sodium

libsodium 1.0.20 modern cryptography suite (Ed25519, XChaCha20-Poly1305 AEAD, BLAKE2b, CSPRNG) for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/sodium` |
| Version | `1.0.20` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/sodium
```

```cp
import {
    SodiumError,
    Ed25519Keypair,
    Ed25519Signature,
    Sodium,
    sodium_init,
    sodium_version_string,
    randombytes_buf,
    crypto_sign_ed25519_keypair,
} from "@cap/sodium";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `SodiumError` | `InitFailed, InvalidKeyLength, InvalidNonceLength, InvalidSignatureLength, InvalidHashLength, BufferTooSmall, CiphertextTooShort, KeypairFailed ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Ed25519Keypair` | `class` | Fields: `pk: [32]u8, sk: [64]u8` |
| `Ed25519Signature` | `class` | Fields: `bytes: [64]u8` |
| `Sodium` | `class` | Methods: `init, versionString, randombytesBuf, ed25519Keypair, ed25519SignDetached, ed25519VerifyDetached, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sodium_init` | `(): i32` | Invokes `sodium_init` and returns `i32`. |
| `sodium_version_string` | `(): [*:0]&u8!?` | Invokes `sodium_version_string` and returns `[*:0]&u8!?`. |
| `randombytes_buf` | `(buf: [*]u8!, size: usize)` | Executes `randombytes_buf` in-place. |
| `crypto_sign_ed25519_keypair` | `(pk: [*]u8!, sk: [*]u8!): i32` | Invokes `crypto_sign_ed25519_keypair` and returns `i32`. |
| `crypto_sign_ed25519_detached` | `(sig: [*]u8!, siglen_p: *u64!?, m: [*]&u8!?, mlen: u64, sk: [*]&u8!): i32` | Invokes `crypto_sign_ed25519_detached` and returns `i32`. |
| `crypto_sign_ed25519_verify_detached` | `(sig: [*]&u8!, m: [*]&u8!?, mlen: u64, pk: [*]&u8!): i32` | Invokes `crypto_sign_ed25519_verify_detached` and returns `i32`. |
| `crypto_aead_xchacha20poly1305_ietf_encrypt` | `(c: [*]u8!, clen_p: *u64!?, m: [*]&u8!?, mlen: u64, ad: [*]&u8!?, adlen: u64, nsec: [*]&u8!?, npub: [*]&u8!, k: [*]&u8!): i32` | Invokes `crypto_aead_xchacha20poly1305_ietf_encrypt` and returns `i32`. |
| `crypto_aead_xchacha20poly1305_ietf_decrypt` | `(m: [*]u8!?, mlen_p: *u64!?, nsec: [*]u8!?, c: [*]&u8!, clen: u64, ad: [*]&u8!?, adlen: u64, npub: [*]&u8!, k: [*]&u8!): i32` | Invokes `crypto_aead_xchacha20poly1305_ietf_decrypt` and returns `i32`. |
| `crypto_generichash` | `(out: [*]u8!, outlen: usize, in_buf: [*]&u8!?, inlen: u64, key: [*]&u8!?, keylen: usize): i32` | Invokes `crypto_generichash` and returns `i32`. |

## Minimal Example

```cp
import { sodium_init } from "@cap/sodium";

pub fn main() {
    let result = sodium_init();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
