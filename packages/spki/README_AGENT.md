# `spki` — AI Agent Quick Ref

Zero-allocation RFC 5280 `SubjectPublicKeyInfo` & `AlgorithmIdentifier` DER/PEM codec (RustCrypto `spki` v0.7 parity).

## Import

```cp
import {
    SPKI_MAX_KEY_LEN, SPKI_PEM_LABEL,
    SPKI_TAG_BIT_STRING, SPKI_TAG_NULL, SPKI_TAG_SEQUENCE,
    SPKI_OK, SPKI_ERR_BUFFER_TOO_SMALL, SPKI_ERR_INVALID_OID, SPKI_ERR_INVALID_DER,
    SPKI_ERR_INVALID_TAG, SPKI_ERR_TRUNCATED, SPKI_ERR_NON_ZERO_UNUSED_BITS,
    SPKI_ERR_KEY_TOO_LONG, SPKI_ERR_INVALID_PARAMS, SPKI_ERR_INVALID_PEM,
    SPKI_ERR_INVALID_PEM_LABEL, SPKI_ERR_TRAILING_DATA,
    AlgorithmIdentifier, AlgIdDecodeResult, SubjectPublicKeyInfo, SpkiDecodeResult,
    encodeAlgorithmId, decodeAlgorithmId, encodeSpkiDer, decodeSpkiDer,
    encodeSpkiPem, decodeSpkiPem, fingerprintSpki,
} from "spki";
```

## Core API Signatures

```cp
// Constants
// SPKI_MAX_KEY_LEN = 160, SPKI_PEM_LABEL = "PUBLIC KEY"
// SPKI_OK = 0; SPKI_ERR_BUFFER_TOO_SMALL = -1 .. SPKI_ERR_TRAILING_DATA = -11

// AlgorithmIdentifier
AlgorithmIdentifier.empty(): AlgorithmIdentifier;
AlgorithmIdentifier.fromOid(oid: ObjectIdentifier): AlgorithmIdentifier;
AlgorithmIdentifier.withNull(oid: ObjectIdentifier): AlgorithmIdentifier;
AlgorithmIdentifier.withParamOid(oid: ObjectIdentifier, param_oid: ObjectIdentifier): AlgorithmIdentifier;
alg.eq(other: AlgorithmIdentifier): bool;

// SubjectPublicKeyInfo { algorithm, key_bytes: [160]u8, key_len: usize, unused_bits: u8 }
SubjectPublicKeyInfo.empty(): SubjectPublicKeyInfo;
SubjectPublicKeyInfo.ed25519(pub_key: []const u8): SpkiDecodeResult;
SubjectPublicKeyInfo.p256(pub_key: []const u8): SpkiDecodeResult;
SubjectPublicKeyInfo.rsa(pub_key_der: []const u8): SpkiDecodeResult;

// Codecs & fingerprint
fn encodeAlgorithmId(alg: AlgorithmIdentifier, out: []u8): i64;
fn decodeAlgorithmId(der: []const u8): AlgIdDecodeResult;
fn encodeSpkiDer(spki: SubjectPublicKeyInfo, out: []u8): i64;
fn decodeSpkiDer(der: []const u8): SpkiDecodeResult;
fn encodeSpkiPem(spki: SubjectPublicKeyInfo, crlf: bool, out: []u8): i64;
fn decodeSpkiPem(pem: string): SpkiDecodeResult;
fn fingerprintSpki(der: []const u8): u64;
```

## Minimal Example

```cp
import { SubjectPublicKeyInfo, encodeSpkiPem, decodeSpkiPem } from "spki";

const built = SubjectPublicKeyInfo.ed25519(raw32);
if (built.ok) {
    let pem: [256]u8 = undefined;
    const n = encodeSpkiPem(built.spki, false, pem[0..256]);
    if (n > 0) { const dec = decodeSpkiPem(pem[0..(n as usize)]); /* dec.ok == true */ }
}
```

## Gotchas & Borrow Rules

- `encode*` functions return `i64`: check `n > 0` before slicing `out[0..(n as usize)]`; negative values are `SPKI_ERR_*`.
- `SubjectPublicKeyInfo.ed25519/p256/rsa` return `SpkiDecodeResult`; always verify `.ok` before using `.spki`.
- Maximum inline key payload is `160` bytes (`SPKI_MAX_KEY_LEN`); larger keys fail with `SPKI_ERR_KEY_TOO_LONG`.
- `decodeSpkiPem` enforces exact `"PUBLIC KEY"` label (`SPKI_ERR_INVALID_PEM_LABEL`).
