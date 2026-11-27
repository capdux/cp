[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# rustls-pemfile

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`rustls-pemfile`](https://github.com/rustls/pemfile) (`v2` / `MIT OR Apache-2.0`), CP package `rustls-pemfile@1.0.0`.

Zero-allocation streaming TLS PEM bundle reader in pure CP. Reuses `pem-rfc7468` for RFC 7468 framing, label validation, and constant-time Base64 decoding.

## Why CP `rustls-pemfile`?

- **Zero Heap Allocation**: `PemBundleReader` maintains only a `(input, offset)` cursor and decodes each PEM block directly into a caller-supplied `der_out: []u8` buffer.
- **Zero-Overhead Stack Scratch**: Internal 64-byte label scratch buffers use `undefined` initialization, eliminating redundant stack zeroing on every `nextItem` / `peekNextKind` call.
- **Full TLS Item Classification**: Recognizes X.509 certificates, PKCS#8 / PKCS#1 / SEC1 private keys, SPKI public keys, CRLs, and CSRs.
- **No-Decode Counting & Peeking**: `countItems()`, `countCerts()`, and `peekNextKind()` inspect PEM framing without decoding Base64 bodies.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`rustls-pemfile`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use rustls_pemfile::{certs, private_key, read_one, Item};
```

**CP (`cap.json` / `main.cp`):**
```typescript
import {
    PemBundleReader, PemItem, readFirstPrivateKey,
    classifyPemLabel, isPrivateKeyKind,
    PEM_ITEM_X509_CERT, PEM_ITEM_PKCS8_KEY, PEM_ITEM_PKCS1_KEY, PEM_ITEM_SEC1_KEY
} from "rustls-pemfile";
```

## Quick Start

### Stream a Certificate Chain & Private Key Bundle

```typescript
import { PemBundleReader, PEM_ITEM_X509_CERT } from "rustls-pemfile";

export fn loadTlsBundle(pem_bundle: string): usize {
    let mut reader = PemBundleReader.new(pem_bundle);
    let mut der_buf: [4096]u8 = undefined;
    let mut cert_count: usize = 0;

    while (!reader.isEof()) {
        let item = reader.nextItem(der_buf[0..4096]);
        if (item.eof) {
            break;
        }
        if (!item.ok) {
            return 0; // malformed PEM or buffer too small
        }
        if (item.isCert()) {
            let cert_der = der_buf[0..item.der_len];
            cert_count += 1;
            _ = cert_der;
        } else if (item.isPrivateKey()) {
            let key_der = der_buf[0..item.der_len];
            _ = key_der;
        }
    }
    return cert_count;
}
```

### Extract the First Private Key from a Mixed Bundle

```typescript
import { readFirstPrivateKey } from "rustls-pemfile";

export fn loadPrivateKey(bundle: string, out: []u8): usize {
    let item = readFirstPrivateKey(bundle, out);
    if (item.ok && item.isPrivateKey()) {
        return item.der_len;
    }
    return 0;
}
```

## API Reference

### Item Kind Constants (`u8`)

| Constant | Value | PEM Label |
| :--- | :---: | :--- |
| `PEM_ITEM_NONE` | `0` | None / EOF / invalid label |
| `PEM_ITEM_X509_CERT` | `1` | `CERTIFICATE` |
| `PEM_ITEM_PKCS8_KEY` | `2` | `PRIVATE KEY` |
| `PEM_ITEM_PKCS1_KEY` | `3` | `RSA PRIVATE KEY` |
| `PEM_ITEM_SEC1_KEY` | `4` | `EC PRIVATE KEY` |
| `PEM_ITEM_SPKI_PUBKEY` | `5` | `PUBLIC KEY` |
| `PEM_ITEM_CRL` | `6` | `X509 CRL` |
| `PEM_ITEM_CSR` | `7` | `CERTIFICATE REQUEST` |
| `PEM_ITEM_UNKNOWN` | `8` | Any other valid RFC 7468 label |

### Types & Functions

- `PemItem` — `{ kind: u8, der_len: usize, ok: bool, eof: bool }` with `makeEof()`, `makeErr()`, `isCert()`, `isPrivateKey()`.
- `PemBundleReader` — `new(input: string)`, `nextItem(der_out: []u8): PemItem`, `peekNextKind(): u8`, `countItems(): usize`, `countCerts(): usize`, `remaining(): usize`, `isEof(): bool`, `reset()`.
- `readFirstPrivateKey(input: string, der_out: []u8): PemItem` — Scans a bundle and decodes the first PKCS#8, PKCS#1, or SEC1 private key.
- `classifyPemLabel(label: string): u8` — Classifies a PEM label string into `PEM_ITEM_*`.
- `isPrivateKeyKind(kind: u8): bool` — Returns `true` for PKCS#8, PKCS#1, or SEC1.

## License

MIT
