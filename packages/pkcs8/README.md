[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# pkcs8

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`pkcs8`](https://github.com/RustCrypto/formats) (`v0.10` / `MIT`), CP package `pkcs8@1.0.0`.

Zero-allocation RFC 5958 PKCS#8 `PrivateKeyInfo` (v1) / `OneAsymmetricKey` (v2) and RFC 8410 Ed25519 / P-256 DER & PEM codec in pure CP. Reuses `spki`, `const-oid`, `pem-rfc7468`, and `subtle`.

## Why CP `pkcs8`?

- **Zero Heap Allocation**: Fixed inline buffers (`private_key: [160]u8`, `public_key: [64]u8`) handle Ed25519, P-256, and other modern asymmetric keys with zero heap allocations.
- **Secure Memory Zeroization**: `PrivateKeyInfo.zeroize()` calls `subtle`'s `zeroizeBytes` to wipe both private and public key buffers safely.
- **Zero-Overhead Stack Initialization**: Stack buffers use `undefined` initialization on hot paths to avoid redundant stack zeroing before writing.
- **RFC 5958 v1 & v2 + RFC 8410**: Supports `PrivateKeyInfo.ed25519(seed32)`, `PrivateKeyInfo.ed25519V2(seed32, pub32)` (with `[1]` publicKey), and `PrivateKeyInfo.p256(ec_priv_der)`.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`pkcs8`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use pkcs8::{PrivateKeyInfo, DecodePrivateKey, EncodePrivateKey};
```

**CP (`cap.json` / `main.cp`):**
```typescript
import {
    PrivateKeyInfo, Pkcs8DecodeResult,
    encodePkcs8Der, decodePkcs8Der, encodePkcs8Pem, decodePkcs8Pem
} from "pkcs8";
```

## Quick Start

### Encode an Ed25519 Seed to PKCS#8 PEM & Zeroize

```typescript
import { PrivateKeyInfo, encodePkcs8Pem } from "pkcs8";

export fn exportEd25519Pem(seed32: &[]u8, pem_out: []u8): i64 {
    let mut built = PrivateKeyInfo.ed25519(seed32);
    if (!built.ok) {
        return built.err;
    }
    defer built.info.zeroize(); // Securely wipe private key bytes on return

    return encodePkcs8Pem(built.info, false, pem_out);
}
```

### Decode PKCS#8 PEM (`-----BEGIN PRIVATE KEY-----`)

```typescript
import { decodePkcs8Pem, PK8_VERSION_V2 } from "pkcs8";

export fn inspectPkcs8Pem(pem: string): i64 {
    let mut res = decodePkcs8Pem(pem);
    if (!res.ok) {
        return res.err;
    }
    defer res.info.zeroize();

    if (res.info.version == PK8_VERSION_V2 && res.info.has_public_key) {
        return res.info.public_key_len as i64;
    }
    return res.info.private_key_len as i64;
}
```

## API Reference

- **`PrivateKeyInfo`**:
  - Fields: `version: u8`, `algorithm: AlgorithmIdentifier`, `private_key: [160]u8`, `private_key_len: usize`, `has_public_key: bool`, `public_key: [64]u8`, `public_key_len: usize`.
  - Constructors & Methods: `empty()`, `ed25519(seed32: &[]u8): Pkcs8DecodeResult`, `ed25519V2(seed32: &[]u8, pub32: &[]u8): Pkcs8DecodeResult`, `p256(ec_priv_der: &[]u8): Pkcs8DecodeResult`, `zeroize()`.
- **DER/PEM Codecs**:
  - `encodePkcs8Der(info: PrivateKeyInfo, out: []u8): i64`
  - `decodePkcs8Der(der: &[]u8): Pkcs8DecodeResult`
  - `encodePkcs8Pem(info: PrivateKeyInfo, crlf: bool, out: []u8): i64`
  - `decodePkcs8Pem(pem: string): Pkcs8DecodeResult`
- **Constants**: `PK8_VERSION_V1 (0)`, `PK8_VERSION_V2 (1)`, `PK8_MAX_PRIVKEY_LEN (160)`, `PK8_MAX_PUBKEY_LEN (64)`, `PK8_PEM_LABEL ("PRIVATE KEY")`, `PK8_OK (0)`, `PK8_ERR_BUFFER_TOO_SMALL (-1)` .. `PK8_ERR_INVALID_SEED (-12)`.

## License

MIT
