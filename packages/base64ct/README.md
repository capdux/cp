[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# base64ct

Zero-allocation **constant-time, table-free, branchless** Base64 encoder and strict RFC 4648 canonical decoder for CP (RustCrypto `base64ct` parity).

Eliminates secret-dependent memory lookups (no 64-byte/256-byte lookup tables) and secret-dependent branches to prevent cache-timing side-channel leaks when encoding/decoding cryptographic keys, tokens, or PEM blocks.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** (also npm `base64-arraybuffer` parity) |
| Original Crate | `base64ct` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/RustCrypto/formats` |
| License | MIT |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: base64ct)
use base64ct::{Base64, Base64UrlUnpadded, Encoding};
let enc = Base64::encode(&secret_key, &mut buf)?;
let dec = Base64UrlUnpadded::decode(jwt_part, &mut out)?;
```

```cp
// CP (cap: base64ct) — table-free branchless arithmetic, zero heap
import { encodeStd, encodeUrlUnpadded, decodeStd, decodeUrlUnpadded } from "base64ct";
const n = encodeStd(secret_key, buf);
const res = decodeUrlUnpadded(jwt_part, out);
```

## Installation

```bash
cap add base64ct
```

## Usage

### Basic Usage: Standard & URL-Safe Constant-Time Encoding/Decoding

```cp
import {
    encodedLen,
    encodeStd,
    encodeUrlUnpadded,
    decodeStd,
    decodeUrlUnpadded
} from "base64ct";

let secret: [5]u8 = ['h', 'e', 'l', 'l', 'o'];

// 1. Compute exact output length and encode with standard alphabet + padding
const need = encodedLen(5, true); // 8
let encBuf: [16]u8 = undefined;
const encLen = encodeStd(secret[0..5], encBuf[0..16]); // encBuf[0..encLen] == "aGVsbG8="

// 2. Decode standard padded Base64 strictly
let decBuf: [16]u8 = undefined;
const r1 = decodeStd("aGVsbG8=", decBuf[0..16]);
if (r1.ok) {
    // decBuf[0..r1.len] == "hello"
}

// 3. URL-safe unpadded (JWT / JWS / WebAuthn)
const uLen = encodeUrlUnpadded(secret[0..5], encBuf[0..16]); // "aGVsbG8"
const r2 = decodeUrlUnpadded("aGVsbG8", decBuf[0..16]);
// r2.ok == true, r2.len == 5
```

### Security & Edge Cases: Canonical Bit Validation & Failure Zeroization

```cp
import { decodeStd, decodeUrlUnpadded } from "base64ct";

let out: [16]u8 = undefined;

// 1. Non-canonical trailing bits are strictly rejected in constant time!
// "aGVsbG9=" has non-zero low 2 bits in '9' before '=' (invalid RFC 4648 canonical encoding)
const badCanon = decodeStd("aGVsbG9=", out[0..16]);
// badCanon.ok == false, badCanon.len == 0, and `out` is automatically zeroed!

// 2. Rejects whitespace, invalid alphabet chars, and bad lengths (len % 4 == 1)
const badLen = decodeUrlUnpadded("a", out[0..16]);
// badLen.ok == false, badLen.len == 0
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `DecodeResult` | `class { len: usize, ok: bool }` | Result of constant-time decoding (`ok == false` zeroes `out`). |
| `encodedLen` | `fn(n: usize, padded: bool): usize` | Exact Base64 output length for `n` input bytes. |
| `encodeStd` | `fn(src: []u8, out: []u8): usize` | Standard alphabet (`+`, `/`) with `=` padding. Returns `0` if `out` too small. |
| `encodeStdUnpadded` | `fn(src: []u8, out: []u8): usize` | Standard alphabet (`+`, `/`) without `=` padding. |
| `encodeUrl` | `fn(src: []u8, out: []u8): usize` | URL-safe alphabet (`-`, `_`) with `=` padding. |
| `encodeUrlUnpadded` | `fn(src: []u8, out: []u8): usize` | URL-safe alphabet (`-`, `_`) without `=` padding (JWT/JOSE). |
| `decodeStd` | `fn(src: string, out: []u8): DecodeResult` | Constant-time strict canonical padded Standard Base64 decoder. |
| `decodeUrlUnpadded` | `fn(src: string, out: []u8): DecodeResult` | Constant-time strict canonical unpadded URL-safe Base64 decoder. |

## Security Properties

- **Table-free & branchless**: Every 6-bit ↔ ASCII conversion uses pure bitwise range arithmetic (`((c + 512) - lo) >> 9`) — zero cache-line lookup tables.
- **Canonical RFC 4648 enforcement**: Verifies unused trailing bits are zero in constant time, preventing malleable encodings.
- **Fail-safe zeroization**: Any decode error wipes the written portion of `out` to `0` before returning `{ len: 0, ok: false }`.

## License

MIT — Clean-room CP implementation with RustCrypto `base64ct` parity.
