# `sec1` — AI Agent Quick Ref

Zero-allocation SEC1 v2.0 EC point codec & RFC 5915 `ECPrivateKey` DER/PEM codec (RustCrypto `sec1` v0.7 parity).

## Import

```cp
import {
    SEC1_IDENTITY, SEC1_COMPRESSED_EVEN, SEC1_COMPRESSED_ODD, SEC1_UNCOMPRESSED,
    SEC1_MAX_POINT_LEN, SEC1_MAX_PRIV_LEN,
    SEC1_P256_COORD_LEN, SEC1_P384_COORD_LEN, SEC1_P521_COORD_LEN,
    SEC1_PEM_LABEL, SEC1_OID_PRIME256V1, SEC1_OID_SECP256K1, SEC1_OID_SECP384R1, SEC1_OID_SECP521R1,
    SEC1_OK, SEC1_ERR_BUFFER_TOO_SMALL, SEC1_ERR_INVALID_POINT_TAG, SEC1_ERR_INVALID_POINT_LEN,
    SEC1_ERR_INVALID_DER, SEC1_ERR_INVALID_VERSION, SEC1_ERR_INVALID_PRIV_KEY,
    SEC1_ERR_INVALID_CURVE_OID, SEC1_ERR_INVALID_PUB_KEY, SEC1_ERR_INVALID_PEM,
    SEC1_ERR_INVALID_PEM_LABEL, SEC1_ERR_TRAILING_DATA,
    PointResult, EncodedPoint, EcPrivateKey, EcPrivateKeyResult,
    makeEcPrivateKey, encodeEcPrivateKeyDer, decodeEcPrivateKeyDer,
    encodeEcPrivateKeyPem, decodeEcPrivateKeyPem,
} from "sec1";
```

## Core API Signatures

```cp
// EncodedPoint { buf: [133]u8, len: usize, coord_size: usize }
EncodedPoint.identity(): EncodedPoint;
EncodedPoint.fromUncompressed(x: []const u8, y: []const u8): PointResult;
EncodedPoint.fromCompressed(x: []const u8, y_is_odd: bool): PointResult;
EncodedPoint.fromBytes(bytes: []const u8): PointResult;
EncodedPoint.fromBytesForCoord(bytes: []const u8, expected_coord_size: usize): PointResult;
pt.tag(): u8;  pt.isIdentity(): bool;  pt.isCompressed(): bool;  pt.isUncompressed(): bool;
pt.xBytes(out: []u8): usize;  pt.yBytes(out: []u8): usize;  pt.toBytes(out: []u8): usize;
pt.compress(): PointResult;   pt.eq(other: EncodedPoint): bool;

// EcPrivateKey { priv_key: [66]u8, priv_len, has_curve, curve_oid, has_pub_key, pub_key }
EcPrivateKey.empty(): EcPrivateKey;
key.zeroize(*_);                                         // wipes priv_key & pub_key; requires `let key`
fn makeEcPrivateKey(priv_bytes: []const u8, has_curve: bool, curve_oid: ObjectIdentifier, has_pub_key: bool, pub_key: EncodedPoint): EcPrivateKeyResult;
fn encodeEcPrivateKeyDer(key: EcPrivateKey, out: []u8): i64;
fn decodeEcPrivateKeyDer(der: []const u8): EcPrivateKeyResult;
fn encodeEcPrivateKeyPem(key: EcPrivateKey, crlf: bool, out: []u8): i64;
fn decodeEcPrivateKeyPem(pem: string): EcPrivateKeyResult;
```

## Minimal Example

```cp
import { decodeEcPrivateKeyPem, encodeEcPrivateKeyDer } from "sec1";

const res = decodeEcPrivateKeyPem(pem_str);
if (res.ok) {
    let key = res.key;
    defer key.zeroize();
    let der: [256]u8 = undefined;
    const n = encodeEcPrivateKeyDer(key, der[0..256]); // n > 0 on success
}
```

## Gotchas & Borrow Rules

- Always bind `let key = res.key; defer key.zeroize();` to wipe private key bytes (`zeroize` takes `*_`).
- `encodeEcPrivateKeyDer` / `encodeEcPrivateKeyPem` return `i64`: check `n > 0` before casting to `usize`.
- `pt.yBytes(out)` returns `0` for compressed or identity points (only uncompressed `0x04` points carry `y`).
- `SEC1_MAX_POINT_LEN = 133` (`1 + 2 * 66` for P-521); `SEC1_MAX_PRIV_LEN = 66`.
