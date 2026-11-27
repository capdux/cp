[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `spki`

Zero-allocation RFC 5280 `SubjectPublicKeyInfo` (SPKI) & `AlgorithmIdentifier` DER/PEM codec in pure CP — a clean-room MIT counterpart to RustCrypto [`spki`](https://github.com/RustCrypto/formats) (v0.7 parity), composing `const-oid` and `pem-rfc7468`.

## Installation & Import

```bash
cap add spki
```

```cp
import {
    SPKI_MAX_KEY_LEN, SPKI_PEM_LABEL,
    SPKI_TAG_BIT_STRING, SPKI_TAG_NULL, SPKI_TAG_SEQUENCE,
    SPKI_OK, SPKI_ERR_BUFFER_TOO_SMALL, SPKI_ERR_INVALID_OID, SPKI_ERR_INVALID_DER,
    SPKI_ERR_INVALID_TAG, SPKI_ERR_TRUNCATED, SPKI_ERR_NON_ZERO_UNUSED_BITS,
    SPKI_ERR_KEY_TOO_LONG, SPKI_ERR_INVALID_PARAMS, SPKI_ERR_INVALID_PEM,
    SPKI_ERR_INVALID_PEM_LABEL, SPKI_ERR_TRAILING_DATA,
    AlgorithmIdentifier, AlgIdDecodeResult, SubjectPublicKeyInfo, SpkiDecodeResult,
    encodeAlgorithmId, decodeAlgorithmId,
    encodeSpkiDer, decodeSpkiDer, encodeSpkiPem, decodeSpkiPem,
    fingerprintSpki,
} from "spki";
```

## Core API

### Constants

| Group | Constants | Values |
| :--- | :--- | :--- |
| Format / Tags | `SPKI_MAX_KEY_LEN` (160), `SPKI_PEM_LABEL` (`"PUBLIC KEY"`), `SPKI_TAG_BIT_STRING` (0x03), `SPKI_TAG_NULL` (0x05), `SPKI_TAG_SEQUENCE` (0x30) | Limits & DER tags |
| Error Codes (`i64`) | `SPKI_OK` (0), `SPKI_ERR_BUFFER_TOO_SMALL` (-1), `SPKI_ERR_INVALID_OID` (-2), `SPKI_ERR_INVALID_DER` (-3), `SPKI_ERR_INVALID_TAG` (-4), `SPKI_ERR_TRUNCATED` (-5), `SPKI_ERR_NON_ZERO_UNUSED_BITS` (-6), `SPKI_ERR_KEY_TOO_LONG` (-7), `SPKI_ERR_INVALID_PARAMS` (-8), `SPKI_ERR_INVALID_PEM` (-9), `SPKI_ERR_INVALID_PEM_LABEL` (-10), `SPKI_ERR_TRAILING_DATA` (-11) | Negative codes on failure |

### Types & Functions

| Symbol | Signature / Methods | Description |
| :--- | :--- | :--- |
| `AlgorithmIdentifier` | `empty()`, `fromOid(oid)`, `withNull(oid)`, `withParamOid(oid, param_oid)`, `eq(other): bool` | RFC 5280 `AlgorithmIdentifier` with optional `NULL` or curve OID parameter |
| `AlgIdDecodeResult` | `ok: bool`, `alg: AlgorithmIdentifier`, `consumed: usize`, `err: i64` | Decoded `AlgorithmIdentifier` + consumed DER length |
| `SubjectPublicKeyInfo` | `empty()`, `ed25519(pub_key: []const u8): SpkiDecodeResult`, `p256(pub_key: []const u8): SpkiDecodeResult`, `rsa(pub_key_der: []const u8): SpkiDecodeResult` | Public key container (`algorithm`, `key_bytes: [160]u8`, `key_len`, `unused_bits`) |
| `SpkiDecodeResult` | `ok: bool`, `spki: SubjectPublicKeyInfo`, `err: i64` | Decode/constructor result (`err == SPKI_OK` on success) |
| `encodeAlgorithmId` / `decodeAlgorithmId` | `(alg, out: []u8): i64` / `(der: []const u8): AlgIdDecodeResult` | DER codec for standalone `AlgorithmIdentifier` |
| `encodeSpkiDer` / `decodeSpkiDer` | `(spki, out: []u8): i64` / `(der: []const u8): SpkiDecodeResult` | DER codec for `SubjectPublicKeyInfo` |
| `encodeSpkiPem` / `decodeSpkiPem` | `(spki, crlf: bool, out: []u8): i64` / `(pem: string): SpkiDecodeResult` | RFC 7468 PEM codec (`-----BEGIN PUBLIC KEY-----`) |
| `fingerprintSpki` | `(der: []const u8): u64` | Fast 64-bit FNV/key-pinning fingerprint of SPKI DER bytes |

## Examples

### 1. Encoding an Ed25519 Public Key to SPKI DER & PEM

```cp
import { print } from "cap:io";
import { SubjectPublicKeyInfo, encodeSpkiDer, encodeSpkiPem, fingerprintSpki } from "spki";

fn exportEd25519(raw_pub: []const u8) {
    const built = SubjectPublicKeyInfo.ed25519(raw_pub);
    if (!built.ok) {
        print("invalid key, err:", built.err);
        return;
    }

    let der: [128]u8 = undefined;
    const der_len = encodeSpkiDer(built.spki, der[0..128]);
    if (der_len > 0) {
        const fp = fingerprintSpki(der[0..(der_len as usize)]);
        print("SPKI DER bytes:", der_len, "fingerprint:", fp);
    }

    let pem: [256]u8 = undefined;
    const pem_len = encodeSpkiPem(built.spki, false, pem[0..256]);
    if (pem_len > 0) {
        print(pem[0..(pem_len as usize)]);
    }
}
```

### 2. Decoding a PEM Public Key & Comparing `AlgorithmIdentifier`

```cp
import { print } from "cap:io";
import { decodeSpkiPem, AlgorithmIdentifier } from "spki";
import { ObjectIdentifier, OID_EC_PUBLIC_KEY, OID_PRIME256V1 } from "const-oid";

fn verifyIsP256(pem: string) {
    const res = decodeSpkiPem(pem);
    if (!res.ok) {
        print("PEM decode error:", res.err);
        return;
    }
    const ec_oid = ObjectIdentifier.fromStr(OID_EC_PUBLIC_KEY).oid;
    const p256_oid = ObjectIdentifier.fromStr(OID_PRIME256V1).oid;
    const expected = AlgorithmIdentifier.withParamOid(ec_oid, p256_oid);
    print("is P-256:", res.spki.algorithm.eq(expected), "key_len:", res.spki.key_len);
}
```

## Differences from Upstream & Performance Highlights

- **Zero Heap Allocation**: `SubjectPublicKeyInfo` stores up to `SPKI_MAX_KEY_LEN` (160) key bytes inline on the stack (covering Ed25519, P-256, P-384, P-521, and compact RSA keys).
- **Strict DER Validation**: Enforces zero unused bits in `BIT STRING` (`SPKI_ERR_NON_ZERO_UNUSED_BITS`), exact PEM label `"PUBLIC KEY"`, and rejects trailing garbage (`SPKI_ERR_TRAILING_DATA`).
- **Built-in Key Pinning Helper**: `fingerprintSpki` computes a fast 64-bit fingerprint directly over SPKI DER bytes.
