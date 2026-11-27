[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `sec1`

Zero-allocation SEC1 v2.0 elliptic curve point encoding & RFC 5915 `ECPrivateKey` DER/PEM codec in pure CP — a clean-room MIT counterpart to RustCrypto [`sec1`](https://github.com/RustCrypto/formats) (v0.7 parity), composing `const-oid`, `pem-rfc7468`, and `subtle`.

## Installation & Import

```bash
cap add sec1
```

```cp
import {
    SEC1_IDENTITY, SEC1_COMPRESSED_EVEN, SEC1_COMPRESSED_ODD, SEC1_UNCOMPRESSED,
    SEC1_MAX_POINT_LEN, SEC1_MAX_PRIV_LEN,
    SEC1_P256_COORD_LEN, SEC1_P384_COORD_LEN, SEC1_P521_COORD_LEN,
    SEC1_PEM_LABEL,
    SEC1_OID_PRIME256V1, SEC1_OID_SECP256K1, SEC1_OID_SECP384R1, SEC1_OID_SECP521R1,
    SEC1_OK, SEC1_ERR_BUFFER_TOO_SMALL, SEC1_ERR_INVALID_POINT_TAG,
    SEC1_ERR_INVALID_POINT_LEN, SEC1_ERR_INVALID_DER, SEC1_ERR_INVALID_VERSION,
    SEC1_ERR_INVALID_PRIV_KEY, SEC1_ERR_INVALID_CURVE_OID, SEC1_ERR_INVALID_PUB_KEY,
    SEC1_ERR_INVALID_PEM, SEC1_ERR_INVALID_PEM_LABEL, SEC1_ERR_TRAILING_DATA,
    PointResult, EncodedPoint, EcPrivateKey, EcPrivateKeyResult,
    makeEcPrivateKey, encodeEcPrivateKeyDer, decodeEcPrivateKeyDer,
    encodeEcPrivateKeyPem, decodeEcPrivateKeyPem,
} from "sec1";
```

## Core API

### Constants

| Group | Constants | Values |
| :--- | :--- | :--- |
| Point Tags (`u8`) | `SEC1_IDENTITY` (0x00), `SEC1_COMPRESSED_EVEN` (0x02), `SEC1_COMPRESSED_ODD` (0x03), `SEC1_UNCOMPRESSED` (0x04) | SEC1 v2.0 point prefixes |
| Sizes & Label | `SEC1_MAX_POINT_LEN` (133), `SEC1_MAX_PRIV_LEN` (66), `SEC1_P256_COORD_LEN` (32), `SEC1_P384_COORD_LEN` (48), `SEC1_P521_COORD_LEN` (66), `SEC1_PEM_LABEL` (`"EC PRIVATE KEY"`) | Supported NIST/SECG curve sizes |
| Curve OIDs | `SEC1_OID_PRIME256V1`, `SEC1_OID_SECP256K1`, `SEC1_OID_SECP384R1`, `SEC1_OID_SECP521R1` | Dotted OID strings |
| Error Codes (`i64`) | `SEC1_OK` (0), `SEC1_ERR_BUFFER_TOO_SMALL` (-1) .. `SEC1_ERR_TRAILING_DATA` (-11) | Negative error codes on failure |

### `EncodedPoint` & `EcPrivateKey`

| Symbol | Signature / Methods | Description |
| :--- | :--- | :--- |
| `EncodedPoint` | `identity()`, `fromUncompressed(x, y): PointResult`, `fromCompressed(x, y_is_odd): PointResult`, `fromBytes(bytes): PointResult`, `fromBytesForCoord(bytes, expected_coord_size): PointResult` | Construct/parse SEC1 EC point (`buf: [133]u8`, `len`, `coord_size`) |
| `EncodedPoint` queries | `tag(): u8`, `isIdentity()`, `isCompressed()`, `isUncompressed()`, `xBytes(out): usize`, `yBytes(out): usize`, `toBytes(out): usize`, `compress(): PointResult`, `eq(other): bool` | Inspect coordinates, serialize, or compress uncompressed points |
| `EcPrivateKey` | `empty()`, `zeroize(*_)` | RFC 5915 private key (`priv_key: [66]u8`, `priv_len`, `curve_oid`, `pub_key`); `zeroize` securely wipes secret bytes |
| `makeEcPrivateKey` | `(priv_bytes: []const u8, has_curve: bool, curve_oid: ObjectIdentifier, has_pub_key: bool, pub_key: EncodedPoint): EcPrivateKeyResult` | Validated constructor for `EcPrivateKey` |
| `encodeEcPrivateKeyDer` / `decodeEcPrivateKeyDer` | `(key, out: []u8): i64` / `(der: []const u8): EcPrivateKeyResult` | RFC 5915 DER codec |
| `encodeEcPrivateKeyPem` / `decodeEcPrivateKeyPem` | `(key, crlf: bool, out: []u8): i64` / `(pem: string): EcPrivateKeyResult` | RFC 7468 PEM codec (`-----BEGIN EC PRIVATE KEY-----`) |

## Examples

### 1. Compressing an Uncompressed P-256 SEC1 Point

```cp
import { print } from "cap:io";
import { EncodedPoint } from "sec1";

fn compressPoint(x: []const u8, y: []const u8) {
    const res = EncodedPoint.fromUncompressed(x, y);
    if (!res.ok) {
        print("invalid point, err:", res.err);
        return;
    }
    const comp = res.point.compress();
    if (comp.ok) {
        let out: [33]u8 = undefined;
        const n = comp.point.toBytes(out[0..33]);
        print("compressed tag:", comp.point.tag(), "len:", n);
    }
}
```

### 2. Decoding an `EC PRIVATE KEY` PEM & Zeroizing Secrets on Exit

```cp
import { print } from "cap:io";
import { decodeEcPrivateKeyPem, encodeEcPrivateKeyDer } from "sec1";

fn reencodePrivateKey(pem: string) {
    const dec = decodeEcPrivateKeyPem(pem);
    if (!dec.ok) {
        print("decode failed, err:", dec.err);
        return;
    }
    // Bind with `let` so `defer key.zeroize()` can wipe secret material
    let key = dec.key;
    defer key.zeroize();

    let der: [256]u8 = undefined;
    const n = encodeEcPrivateKeyDer(key, der[0..256]);
    print("DER length:", n, "has_curve:", key.has_curve);
}
```

## Differences from Upstream & Performance Highlights

- **Zero Heap Allocation & Secure Zeroization**: `EcPrivateKey` stores up to 66-byte private scalars (P-256/secp256k1/P-384/P-521) on the stack and calls `subtle.zeroizeBytes` in `key.zeroize()` to prevent secret retention.
- **Deterministic Error Codes**: All encoding and decoding APIs return `i64` or `*Result` structs with explicit `SEC1_ERR_*` codes instead of panicking or throwing.
- **Strict RFC 5915 Compliance**: Validates version (`1`), private key length, optional `[0]` curve OID, optional `[1]` public key bit string, and rejects trailing bytes.
