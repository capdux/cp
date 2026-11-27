[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `x509-cert`

Zero-allocation RFC 5280 X.509 v1/v2/v3 certificate DER & PEM codec in pure CP — a clean-room MIT counterpart to RustCrypto [`x509-cert`](https://github.com/RustCrypto/formats) (v0.2 parity), composing `der`, `const-oid`, `spki`, and `pem-rfc7468`.

## Installation & Import

```bash
cap add x509-cert
```

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

## Core API

### Constants

| Group | Constants | Values |
| :--- | :--- | :--- |
| Format / Limits | `X509_PEM_LABEL`, `X509_MAX_SERIAL_LEN`, `X509_MAX_SIG_LEN` | `"CERTIFICATE"`, `20`, `128` |
| Status & Error Codes (`i64`) | `X509_OK` (0), `X509_ERR_BUFFER_TOO_SMALL` (-1), `X509_ERR_INVALID_DER` (-2), `X509_ERR_INVALID_VERSION` (-3), `X509_ERR_INVALID_SERIAL` (-4), `X509_ERR_INVALID_ALG` (-5), `X509_ERR_ALG_MISMATCH` (-6), `X509_ERR_INVALID_NAME` (-7), `X509_ERR_INVALID_VALIDITY` (-8), `X509_ERR_INVALID_SPKI` (-9), `X509_ERR_INVALID_SIGNATURE` (-10), `X509_ERR_INVALID_PEM` (-11), `X509_ERR_INVALID_PEM_LABEL` (-12), `X509_ERR_TRAILING_DATA` (-13) | Negative error codes returned by `encode*` or `.err` |

### Types & Codec Functions

| Symbol | Signature / Fields | Description |
| :--- | :--- | :--- |
| `X509Validity` | `empty()`, `fromStrs(nb: string, na: string): X509Validity` | Validity window (`UTCTime` / `GeneralizedTime`, 20-byte inline buffers) |
| `X509Name` | `empty()`, `init(cn: string, org: string, country: string): X509Name`, `eq(other: X509Name): bool` | Distinguished Name (`CN`, `O`, `C`) with fixed inline buffers |
| `X509Certificate` | `version`, `serial`, `signature_alg: AlgorithmIdentifier`, `issuer`, `validity`, `subject`, `spki: SubjectPublicKeyInfo`, `signature`; `empty()` | Full stack-allocated RFC 5280 certificate |
| `X509DecodeResult` | `ok: bool`, `cert: X509Certificate`, `err: i64` | Decode result container (`err == X509_OK` on success) |
| `encodeX509Der` | `(cert: X509Certificate, out: []u8): i64` | Encode certificate to DER; returns byte length (`> 0`) or negative `X509_ERR_*` |
| `decodeX509Der` | `(der: []const u8): X509DecodeResult` | Strictly decode DER certificate (rejects mismatched signature algorithms & trailing bytes) |
| `encodeX509Pem` | `(cert: X509Certificate, crlf: bool, out: []u8): i64` | Encode certificate to RFC 7468 PEM (`-----BEGIN CERTIFICATE-----`) |
| `decodeX509Pem` | `(pem: string): X509DecodeResult` | Decode PEM certificate and validate `"CERTIFICATE"` label |
| `isSelfSigned` | `(cert: X509Certificate): bool` | Return `true` if `cert.issuer.eq(cert.subject)` |

## Examples

### 1. Decoding a PEM Certificate and Inspecting Subject / Self-Signed Status

```cp
import { print } from "cap:io";
import { decodeX509Pem, isSelfSigned } from "x509-cert";

fn inspectCert(pem: string) {
    const res = decodeX509Pem(pem);
    if (!res.ok) {
        print("decode failed, err:", res.err);
        return;
    }
    const cert = res.cert;
    print("version:", cert.version, "self_signed:", isSelfSigned(cert));
    print("CN:", cert.subject.cn[0..cert.subject.cn_len]);
    print("Org:", cert.subject.org[0..cert.subject.org_len]);
}
```

### 2. Building and Encoding an X.509 Certificate to DER & PEM

```cp
import { print } from "cap:io";
import {
    X509Certificate, X509Name, X509Validity,
    encodeX509Der, encodeX509Pem,
} from "x509-cert";
import { AlgorithmIdentifier, SubjectPublicKeyInfo } from "spki";
import { ObjectIdentifier, OID_ED25519 } from "const-oid";

fn buildCert(pub_key: []const u8) {
    let cert = X509Certificate.empty();
    cert.version = 2; // v3
    cert.serial[0] = 0x01;
    cert.serial_len = 1;

    const ed_oid = ObjectIdentifier.fromStr(OID_ED25519);
    cert.signature_alg = AlgorithmIdentifier.fromOid(ed_oid.oid);
    cert.issuer = X509Name.init("root.cp-lang.org", "CP Foundation", "US");
    cert.subject = cert.issuer;
    cert.validity = X509Validity.fromStrs("260101000000Z", "360101000000Z");

    const spki_res = SubjectPublicKeyInfo.ed25519(pub_key);
    if (spki_res.ok) { cert.spki = spki_res.spki; }
    cert.signature_len = 64;

    let der_buf: [1024]u8 = undefined;
    const der_len = encodeX509Der(cert, der_buf[0..1024]);
    let pem_buf: [2048]u8 = undefined;
    const pem_len = encodeX509Pem(cert, false, pem_buf[0..2048]);
    print("der_len:", der_len, "pem_len:", pem_len);
}
```

## Differences from Upstream & Performance Highlights

- **Zero Heap Allocation**: All certificate fields (`serial`, `X509Name`, `X509Validity`, `SubjectPublicKeyInfo`, `signature`) use fixed-size inline buffers — no `Vec` or `Box` allocations during TLS handshake inspection.
- **Strict RFC 5280 Validation**: Verifies that outer `signatureAlgorithm` matches `tbsCertificate.signature` (`X509_ERR_ALG_MISMATCH`) and rejects trailing bytes (`X509_ERR_TRAILING_DATA`).
- **Composable Crypto Format Stack**: Directly reuses `der`, `const-oid`, `spki`, and `pem-rfc7468` with no duplicated ASN.1 logic.
