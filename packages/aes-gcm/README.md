[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# aes-gcm

Pure CP, zero-allocation **NIST SP 800-38D AES-128-GCM** and **AES-256-GCM** AEAD authenticated encryption, composing `aes` and `ghash-gf128` with constant-time 16-byte tag verification.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [crates.io](https://crates.io/crates/aes-gcm) |
| **Upstream Package** | `aes-gcm` (RustCrypto AEADs, v0.10 parity) |
| **Upstream Repo** | https://github.com/RustCrypto/AEADs |
| **CP Package** | `aes-gcm` (`1.0.0`) |
| **License** | Apache-2.0 OR MIT |

Keep the exact same package name in CP — no renaming required:

```rust
// Rust (crates.io)
use aes_gcm::{Aes128Gcm, Aes256Gcm, KeyInit, aead::Aead};
```

```typescript
// CP (cap)
import { Aes128Gcm, Aes256Gcm } from "aes-gcm";
```

## Installation

```bash
cap add aes-gcm
```

## Key Features & Security Highlights

- **NIST SP 800-38D Compliant**: Full 96-bit IV (12-byte nonce) GCTR encryption + GHASH authentication over AAD and ciphertext.
- **Zero Heap Allocation**: `Aes128Gcm` and `Aes256Gcm` hold pre-expanded round keys and GHASH subkey `H = AES_K(0^128)` directly by value on the stack.
- **Constant-Time Verification**: `decrypt` verifies the 16-byte authentication tag using `agcmCtEq16` before decrypting ciphertext, returning `false` on any bit flip in AAD, ciphertext, or tag.
- **Reusable Cipher Instances**: Expand the AES key schedule once via `Aes128Gcm.new(key)` / `Aes256Gcm.new(key)` and encrypt/decrypt any number of messages with unique nonces.

## Quick Start

### AES-128-GCM Encrypt & Decrypt

```typescript
import { Aes128Gcm } from "aes-gcm";

fn main(): void {
    const key: [16]u8 = [
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
    ];
    const nonce: [12]u8 = [
        0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
        0xde, 0xca, 0xf8, 0x88,
    ];
    const aad: &[]u8 = "session-aad";
    const plaintext: &[]u8 = "Hello AES-128-GCM!";

    const gcm = Aes128Gcm.new(key);

    let ct: [18]u8 = undefined;
    const tag = gcm.encrypt(nonce, aad, plaintext, ct[0..]);

    let pt: [18]u8 = undefined;
    const ok = gcm.decrypt(nonce, aad, ct[0..], tag, pt[0..]);
    if (!ok) {
        // Tag mismatch: reject message
        return;
    }
}
```

### AES-256-GCM Encrypt & Decrypt

```typescript
import { Aes256Gcm } from "aes-gcm";

fn encryptMessage256(
    key: [32]u8,
    nonce: [12]u8,
    aad: &[]u8,
    plaintext: &[]u8,
    out_ct: []u8,
): [16]u8 {
    const gcm = Aes256Gcm.new(key);
    return gcm.encrypt(nonce, aad, plaintext, out_ct);
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Aes128Gcm.new` | `(key: [16]u8) -> Aes128Gcm` | Initialize AES-128-GCM cipher and precompute GHASH subkey |
| `Aes128Gcm.encrypt` | `(nonce: [12]u8, aad: &[]u8, plaintext: &[]u8, out_ct: []u8) -> [16]u8` | Encrypt `plaintext` into `out_ct` and return 16-byte tag |
| `Aes128Gcm.decrypt` | `(nonce: [12]u8, aad: &[]u8, ciphertext: &[]u8, tag: [16]u8, out_pt: []u8) -> bool` | Constant-time verify tag and decrypt into `out_pt` |
| `Aes128Gcm.gctr` | `(nonce: [12]u8, input: &[]u8, output: []u8)` | Low-level GCM counter mode transform |
| `Aes256Gcm.new` | `(key: [32]u8) -> Aes256Gcm` | Initialize AES-256-GCM cipher and precompute GHASH subkey |
| `Aes256Gcm.encrypt` | `(nonce: [12]u8, aad: &[]u8, plaintext: &[]u8, out_ct: []u8) -> [16]u8` | Encrypt `plaintext` into `out_ct` and return 16-byte tag |
| `Aes256Gcm.decrypt` | `(nonce: [12]u8, aad: &[]u8, ciphertext: &[]u8, tag: [16]u8, out_pt: []u8) -> bool` | Constant-time verify tag and decrypt into `out_pt` |
| `Aes256Gcm.gctr` | `(nonce: [12]u8, input: &[]u8, output: []u8)` | Low-level GCM counter mode transform |
| `agcmCtEq16` | `(a: [16]u8, b: [16]u8) -> bool` | Constant-time 16-byte tag comparison |
| `agcmComputeTag` | `(h: [16]u8, e_j0: [16]u8, aad: &[]u8, ct: &[]u8) -> [16]u8` | Compute 16-byte GHASH authentication tag |
| `agcmMakeCounterBlock` | `(nonce: [12]u8, counter: u32) -> [16]u8` | Build 16-byte GCM counter block `nonce \|\| BE32(counter)` |
| `agcmZeroBlock` | `() -> [16]u8` | Return 16-byte all-zero block |

## License

Apache-2.0 OR MIT
