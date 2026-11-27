# `x509-cert` — AI Agent Quick Ref

Zero-allocation RFC 5280 X.509 v1/v2/v3 DER & PEM codec (RustCrypto `x509-cert` v0.2 parity; composes `der`, `const-oid`, `spki`, `pem-rfc7468`).

## Import

```cp
import {
    X509_PEM_LABEL, X509_MAX_SERIAL_LEN, X509_MAX_SIG_LEN,
    X509_OK, X509_ERR_BUFFER_TOO_SMALL, X509_ERR_INVALID_DER, X509_ERR_INVALID_VERSION,
    X509_ERR_INVALID_SERIAL, X509_ERR_INVALID_ALG, X509_ERR_ALG_MISMATCH,
    X509_ERR_INVALID_NAME, X509_ERR_INVALID_VALIDITY, X509_ERR_INVALID_SPKI,
    X509_ERR_INVALID_SIGNATURE, X509_ERR_INVALID_PEM, X509_ERR_INVALID_PEM_LABEL,
    X509_ERR_TRAILING_DATA,
    X509Validity, X509Name, X509Certificate, X509DecodeResult,
    encodeX509Der, decodeX509Der, encodeX509Pem, decodeX509Pem, isSelfSigned,
} from "x509-cert";
```

## Core API Signatures

```cp
// Constants
// X509_PEM_LABEL = "CERTIFICATE", X509_MAX_SERIAL_LEN = 20, X509_MAX_SIG_LEN = 128
// X509_OK = 0; X509_ERR_BUFFER_TOO_SMALL = -1 .. X509_ERR_TRAILING_DATA = -13

// Structs & constructors
X509Validity.empty(): X509Validity;
X509Validity.fromStrs(nb: string, na: string): X509Validity;
X509Name.empty(): X509Name;
X509Name.init(cn: string, org: string, country: string): X509Name;
name.eq(other: X509Name): bool;
X509Certificate.empty(): X509Certificate;
struct X509DecodeResult { ok: bool, cert: X509Certificate, err: i64 }

// Codec & helpers
fn encodeX509Der(cert: X509Certificate, out: []u8): i64;              // >0 bytes or negative err
fn decodeX509Der(der: []const u8): X509DecodeResult;
fn encodeX509Pem(cert: X509Certificate, crlf: bool, out: []u8): i64;  // >0 bytes or negative err
fn decodeX509Pem(pem: string): X509DecodeResult;
fn isSelfSigned(cert: X509Certificate): bool;
```

## Minimal Example

```cp
import { decodeX509Pem, encodeX509Der, isSelfSigned } from "x509-cert";

const res = decodeX509Pem(pem_str);
if (res.ok) {
    let der: [1024]u8 = undefined;
    const n = encodeX509Der(res.cert, der[0..1024]);
    if (n > 0 && isSelfSigned(res.cert)) { /* der[0..(n as usize)] */ }
}
```

## Gotchas & Borrow Rules

- `encodeX509Der` / `encodeX509Pem` return `i64`: check `n > 0` before casting `(n as usize)`; negative values are `X509_ERR_*`.
- `X509Certificate.version` follows ASN.1 encoding: `0` = v1, `1` = v2, `2` = v3.
- Inline field caps: `serial` max 20 bytes, `cn`/`org` max 64 bytes, `country` max 8 bytes, `signature` max 128 bytes.
- `decodeX509Der` rejects mismatched outer/inner signature algorithms (`X509_ERR_ALG_MISMATCH`) and trailing bytes (`X509_ERR_TRAILING_DATA`).
