[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# pkcs1

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`pkcs1`](https://github.com/RustCrypto/formats) (`v0.7` / `MIT`), CP package `pkcs1@1.0.0`.

Zero-allocation RFC 8017 Appendix A.1.1 (`RSAPublicKey`) and Appendix A.1.2 (`RSAPrivateKey`) DER/PEM codec in pure CP. Reuses `der`, `pem-rfc7468`, and `subtle` for strict ASN.1 DER parsing, constant-time Base64 PEM framing, and secure key zeroization.

## Why CP `pkcs1`?

- **Zero Heap Allocation**: Supports up to 4096-bit RSA keys (`n` up to 512 bytes, primes up to 256 bytes) completely on stack/inline buffers.
- **Secure Memory Zeroization**: `RsaPrivateKey.zeroize()` calls `subtle`'s `zeroizeBytes` to wipe `n`, `d`, `p`, `q`, `dp`, `dq`, and `qinv` without compiler dead-store elimination.
- **Zero-Overhead Stack Initialization**: Large fixed key buffers (`[512]u8`, `[256]u8`, `[513]u8` DER scratch) use `undefined` initialization on hot paths to avoid redundant stack zeroing.
- **Strict X.690 DER & RFC 7468 PEM**: Rejects negative integers, non-canonical DER lengths, trailing garbage, and mismatched PEM labels (`RSA PUBLIC KEY` / `RSA PRIVATE KEY`).

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`pkcs1`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use pkcs1::{DecodeRsaPrivateKey, EncodeRsaPublicKey, RsaPrivateKey, RsaPublicKey};
```

**CP (`cap.json` / `main.cp`):**
```typescript
import {
    RsaPublicKey, RsaPrivateKey, modulusBitLen,
    encodeRsaPublicKeyDer, decodeRsaPublicKeyDer,
    encodeRsaPublicKeyPem, decodeRsaPublicKeyPem,
    encodeRsaPrivateKeyDer, decodeRsaPrivateKeyDer,
    encodeRsaPrivateKeyPem, decodeRsaPrivateKeyPem
} from "pkcs1";
```

## Quick Start

### Decode an RSA Private Key PEM, Derive Public Key, and Zeroize

```typescript
import {
    decodeRsaPrivateKeyPem, encodeRsaPublicKeyPem, modulusBitLen
} from "pkcs1";

export fn extractPublicPem(priv_pem: string, pub_pem_out: []u8): i64 {
    let mut res = decodeRsaPrivateKeyPem(priv_pem);
    if (!res.ok) {
        return res.err;
    }
    defer res.key.zeroize(); // Securely wipe private key material on scope exit

    let pub_key = res.key.publicKey();
    let bits = modulusBitLen(pub_key);
    if (bits < 2048) {
        return -100; // Enforce minimum RSA key size
    }
    return encodeRsaPublicKeyPem(pub_key, false, pub_pem_out);
}
```

### Construct & Encode `RsaPublicKey` to DER

```typescript
import { RsaPublicKey, encodeRsaPublicKeyDer } from "pkcs1";

export fn buildPubDer(modulus: &[]u8, out: []u8): i64 {
    let res = RsaPublicKey.new(modulus, 65537);
    if (!res.ok) {
        return res.err;
    }
    return encodeRsaPublicKeyDer(res.key, out);
}
```

## API Reference

- **`RsaPublicKey`**: `n: [512]u8`, `n_len: usize`, `e: u64`; methods `empty()`, `new(n_bytes: &[]u8, e: u64): RsaPublicKeyResult`.
- **`RsaPrivateKey`**: `version: u8`, `n`, `e`, `d`, `p`, `q`, `dp`, `dq`, `qinv` (with `*_len`); methods `empty()`, `publicKey(): RsaPublicKey`, `zeroize()`.
- **Utilities**: `modulusBitLen(pub_key: RsaPublicKey): usize`.
- **DER/PEM Codecs**:
  - `encodeRsaPublicKeyDer(pub_key, out): i64` / `decodeRsaPublicKeyDer(der): RsaPublicKeyResult`
  - `encodeRsaPublicKeyPem(pub_key, crlf, out): i64` / `decodeRsaPublicKeyPem(pem): RsaPublicKeyResult`
  - `encodeRsaPrivateKeyDer(priv_key, out): i64` / `decodeRsaPrivateKeyDer(der): RsaPrivateKeyResult`
  - `encodeRsaPrivateKeyPem(priv_key, crlf, out): i64` / `decodeRsaPrivateKeyPem(pem): RsaPrivateKeyResult`
- **Error Codes**: `PK1_OK (0)`, `PK1_ERR_BUFFER_TOO_SMALL (-1)`, `PK1_ERR_INVALID_DER (-2)`, `PK1_ERR_INVALID_VERSION (-3)`, `PK1_ERR_INVALID_INTEGER (-4)`, `PK1_ERR_NEGATIVE_INTEGER (-5)`, `PK1_ERR_KEY_TOO_LONG (-6)`, `PK1_ERR_TRAILING_DATA (-7)`, `PK1_ERR_INVALID_PEM (-8)`, `PK1_ERR_INVALID_PEM_LABEL (-9)`.

## License

MIT
