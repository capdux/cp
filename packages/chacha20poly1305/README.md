[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# chacha20poly1305

Pure CP, zero-allocation **RFC 8439 ChaCha20-Poly1305** and **XChaCha20-Poly1305** AEAD authenticated encryption cipher, composing `poly1305`, `rand_chacha`, and `subtle` with constant-time tag verification and automatic key-material zeroization.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [crates.io](https://crates.io/crates/chacha20poly1305) |
| **Upstream Package** | `chacha20poly1305` (RustCrypto AEADs, v0.10 parity) |
| **Upstream Repo** | https://github.com/RustCrypto/AEADs |
| **CP Package** | `chacha20poly1305` (`1.0.0`) |
| **License** | MIT / Apache-2.0 |

Keep the exact same package name in CP — no renaming required:

```rust
// Rust (crates.io)
use chacha20poly1305::{ChaCha20Poly1305, XChaCha20Poly1305, aead::Aead};
```

```typescript
// CP (cap)
import {
    aeadEncrypt,
    aeadDecrypt,
    xchacha20Poly1305Encrypt,
    xchacha20Poly1305Decrypt,
    CHACHA20_KEY_LEN,
    CHACHA20_NONCE_LEN,
    CHACHA20_TAG_LEN,
    XCHACHA20_NONCE_LEN,
} from "chacha20poly1305";
```

## Installation

```bash
cap add chacha20poly1305
```

## Key Features & Security Highlights

- **Zero Heap Allocation**: All encryption, decryption, Poly1305 one-time key derivation, and HChaCha20 subkey derivation operate on caller-provided fixed buffers.
- **Constant-Time Tag Verification**: `aeadDecrypt` and `xchacha20Poly1305Decrypt` verify the 16-byte Poly1305 MAC in constant time (`ctEqBytes`) before exposing plaintext, and clear `plaintext` on failure.
- **RFC 8439 + XChaCha20 Extended Nonces**: Supports standard 12-byte (96-bit) nonces as well as 24-byte (192-bit) nonces via `xchacha20HChaCha20` for safe random-nonce generation without collision risk.
- **Key Material Zeroization**: Ephemeral Poly1305 one-time keys and HChaCha20 subkeys are wiped via `zeroizeBytes` immediately after use.

## Quick Start

### ChaCha20-Poly1305 AEAD (12-byte Nonce)

```typescript
import {
    aeadEncrypt,
    aeadDecrypt,
    CHACHA20_TAG_LEN,
} from "chacha20poly1305";

fn main(): void {
    const key: [32]u8 = [
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    ];
    const nonce: [12]u8 = [0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47];
    const aad: &[]u8 = "header-v1";
    const plaintext: &[]u8 = "secret agent payload";

    let ciphertext: [20]u8 = undefined;
    let tag: [CHACHA20_TAG_LEN]u8 = undefined;

    const enc_ok = aeadEncrypt(key[0..], nonce[0..], aad, plaintext, ciphertext[0..], tag[0..]);
    if (!enc_ok) {
        return;
    }

    let decrypted: [20]u8 = undefined;
    const dec_ok = aeadDecrypt(key[0..], nonce[0..], aad, ciphertext[0..], tag[0..], decrypted[0..]);
    if (!dec_ok) {
        // Authentication failed: tampered ciphertext, tag, or AAD
        return;
    }
}
```

### XChaCha20-Poly1305 AEAD (24-byte Nonce)

```typescript
import {
    xchacha20Poly1305Encrypt,
    xchacha20Poly1305Decrypt,
} from "chacha20poly1305";

fn sealWithRandomNonce(
    key: &[]u8,
    nonce24: &[]u8,
    aad: &[]u8,
    plaintext: &[]u8,
    out_ct: []u8,
    out_tag: []u8,
): bool {
    return xchacha20Poly1305Encrypt(key, nonce24, aad, plaintext, out_ct, out_tag);
}

fn openWithRandomNonce(
    key: &[]u8,
    nonce24: &[]u8,
    aad: &[]u8,
    ciphertext: &[]u8,
    tag: &[]u8,
    out_pt: []u8,
): bool {
    return xchacha20Poly1305Decrypt(key, nonce24, aad, ciphertext, tag, out_pt);
}
```

### Low-Level Stream & Subkey Primitives

```typescript
import { chacha20Block, chacha20Xor, poly1305KeyGen, xchacha20HChaCha20 } from "chacha20poly1305";

fn deriveSubkey(key32: &[]u8, nonce16: &[]u8, out_subkey: []u8): bool {
    return xchacha20HChaCha20(key32, nonce16, out_subkey);
}
```

## API Overview

| Symbol | Signature / Value | Description |
| :--- | :--- | :--- |
| `CHACHA20_KEY_LEN` | `32` | Required key length in bytes |
| `CHACHA20_NONCE_LEN` | `12` | RFC 8439 ChaCha20-Poly1305 nonce length |
| `CHACHA20_BLOCK_LEN` | `64` | ChaCha20 keystream block size |
| `CHACHA20_TAG_LEN` | `16` | Poly1305 authentication tag length |
| `XCHACHA20_NONCE_LEN` | `24` | XChaCha20-Poly1305 extended nonce length |
| `aeadEncrypt` | `(key, nonce, aad, plaintext, ciphertext, tag) -> bool` | RFC 8439 ChaCha20-Poly1305 encrypt + tag |
| `aeadDecrypt` | `(key, nonce, aad, ciphertext, tag, plaintext) -> bool` | Constant-time verify + decrypt |
| `xchacha20Poly1305Encrypt` | `(key, nonce24, aad, plaintext, ciphertext, tag) -> bool` | XChaCha20-Poly1305 encrypt + tag |
| `xchacha20Poly1305Decrypt` | `(key, nonce24, aad, ciphertext, tag, plaintext) -> bool` | XChaCha20-Poly1305 constant-time verify + decrypt |
| `xchacha20HChaCha20` | `(key, nonce16, out_subkey) -> bool` | Derive 32-byte HChaCha20 subkey |
| `chacha20Block` | `(key, counter, nonce, out) -> bool` | Generate 64-byte ChaCha20 keystream block |
| `chacha20Xor` | `(key, counter, nonce, input, out) -> bool` | XOR stream cipher with initial block counter |
| `poly1305KeyGen` | `(key, nonce, out) -> bool` | Generate 32-byte RFC 8439 Poly1305 one-time key |

## License

MIT / Apache-2.0
