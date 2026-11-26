// @cap/sec1 — Pure CP zero-allocation SEC1 v2.0 Elliptic Curve Point & RFC 5915 ECPrivateKey
// Clean-room MIT implementation with parity to RustCrypto `sec1`.
// Reuses:
//   - `ObjectIdentifier`, `OID_TAG`, `OID_PRIME256V1` from `@cap/const-oid`
//   - `encodePem`, `decodePem` from `@cap/pem-rfc7468` (`EC PRIVATE KEY` PEM)
//   - `zeroizeBytes` from `@cap/subtle-ct` (secure private key zeroization)

import { OID_TAG, OID_PRIME256V1, ObjectIdentifier } from "../../const-oid/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";
import { zeroizeBytes } from "../../subtle-ct/src/index.cp";

// SEC1 v2.0 §2.3.3 / §2.3.4 Point Tag constants
pub const SEC1_IDENTITY: u8 = 0x00;
pub const SEC1_COMPRESSED_EVEN: u8 = 0x02;
pub const SEC1_COMPRESSED_ODD: u8 = 0x03;
pub const SEC1_UNCOMPRESSED: u8 = 0x04;

pub const SEC1_MAX_POINT_LEN: usize = 133;
pub const SEC1_MAX_PRIV_LEN: usize = 66;
pub const SEC1_P256_COORD_LEN: usize = 32;
pub const SEC1_P384_COORD_LEN: usize = 48;
pub const SEC1_P521_COORD_LEN: usize = 66;
pub const SEC1_PEM_LABEL: string = "EC PRIVATE KEY";

// Standard elliptic curve OIDs
pub const SEC1_OID_PRIME256V1: string = OID_PRIME256V1;
pub const SEC1_OID_SECP256K1: string = "1.3.132.0.10";
pub const SEC1_OID_SECP384R1: string = "1.3.132.0.34";
pub const SEC1_OID_SECP521R1: string = "1.3.132.0.35";

// ASN.1 DER Tags
const SEC1_TAG_INTEGER: u8 = 0x02;
const SEC1_TAG_BIT_STRING: u8 = 0x03;
const SEC1_TAG_OCTET_STRING: u8 = 0x04;
const SEC1_TAG_SEQUENCE: u8 = 0x30;
const SEC1_TAG_CTX0_EXPLICIT: u8 = 0xA0;
const SEC1_TAG_CTX1_EXPLICIT: u8 = 0xA1;

// Error codes
pub const SEC1_OK: i64 = 0;
pub const SEC1_ERR_BUFFER_TOO_SMALL: i64 = -1;
pub const SEC1_ERR_INVALID_POINT_TAG: i64 = -2;
pub const SEC1_ERR_INVALID_POINT_LEN: i64 = -3;
pub const SEC1_ERR_INVALID_DER: i64 = -4;
pub const SEC1_ERR_INVALID_VERSION: i64 = -5;
pub const SEC1_ERR_INVALID_PRIV_KEY: i64 = -6;
pub const SEC1_ERR_INVALID_CURVE_OID: i64 = -7;
pub const SEC1_ERR_INVALID_PUB_KEY: i64 = -8;
pub const SEC1_ERR_INVALID_PEM: i64 = -9;
pub const SEC1_ERR_INVALID_PEM_LABEL: i64 = -10;
pub const SEC1_ERR_TRAILING_DATA: i64 = -11;

struct Sec1LenDecode {
    ok: bool,
    val_len: usize,
    next_pos: usize,
}

fn sec1ZeroPointBuf(): [133]u8 {
    let b: [133]u8 = undefined;
    let i: usize = 0;
    while (i < 133) {
        b[i] = 0 as u8;
        i = i + 1;
    }
    return b;
}

fn sec1ZeroPrivBuf(): [66]u8 {
    let b: [66]u8 = undefined;
    let i: usize = 0;
    while (i < 66) {
        b[i] = 0 as u8;
        i = i + 1;
    }
    return b;
}

fn sec1DerLenSize(len: usize): usize {
    if (len < 128) {
        return 1;
    }
    if (len < 256) {
        return 2;
    }
    return 3;
}

fn sec1WriteDerLen(out: []u8, pos: usize, len: usize): usize {
    if (len < 128) {
        out[pos] = len as u8;
        return pos + 1;
    }
    if (len < 256) {
        out[pos] = 0x81 as u8;
        out[pos + 1] = len as u8;
        return pos + 2;
    }
    out[pos] = 0x82 as u8;
    out[pos + 1] = ((len >> (8 as u6)) & 0xFF) as u8;
    out[pos + 2] = (len & 0xFF) as u8;
    return pos + 3;
}

fn sec1ReadDerLen(der: []const u8, pos: usize): Sec1LenDecode {
    if (pos >= der.len) {
        return Sec1LenDecode { ok: false, val_len: 0, next_pos: pos };
    }
    const b0 = der[pos];
    if ((b0 & (0x80 as u8)) == 0) {
        const vlen = b0 as usize;
        const npos = pos + 1;
        if (npos + vlen > der.len) {
            return Sec1LenDecode { ok: false, val_len: 0, next_pos: pos };
        }
        return Sec1LenDecode { ok: true, val_len: vlen, next_pos: npos };
    }

    const nbytes = (b0 & (0x7F as u8)) as usize;
    if (nbytes == 0 || nbytes > 2) {
        return Sec1LenDecode { ok: false, val_len: 0, next_pos: pos };
    }
    if (pos + 1 + nbytes > der.len) {
        return Sec1LenDecode { ok: false, val_len: 0, next_pos: pos };
    }

    let vlen: usize = 0;
    if (nbytes == 1) {
        vlen = der[pos + 1] as usize;
        if (vlen < 128) {
            return Sec1LenDecode { ok: false, val_len: 0, next_pos: pos };
        }
    } else {
        vlen = ((der[pos + 1] as usize) << (8 as u6)) | (der[pos + 2] as usize);
        if (vlen < 256) {
            return Sec1LenDecode { ok: false, val_len: 0, next_pos: pos };
        }
    }

    const npos = pos + 1 + nbytes;
    if (npos + vlen > der.len) {
        return Sec1LenDecode { ok: false, val_len: 0, next_pos: pos };
    }
    return Sec1LenDecode { ok: true, val_len: vlen, next_pos: npos };
}

fn sec1StrEqBytes(s: string, b: []const u8): bool {
    if (s.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

pub struct PointResult {
    pub ok: bool,
    pub point: EncodedPoint,
    pub err: i64,
}

/// SEC1 v2.0 encoded elliptic curve point (supports P-256, P-384, P-521; zero heap allocation).
pub struct EncodedPoint {
    pub buf: [133]u8,
    pub len: usize,
    pub coord_size: usize,

    /// Construct the point at infinity (identity, encoded as single byte `0x00`).
    pub static fn identity(): EncodedPoint {
        let b = sec1ZeroPointBuf();
        b[0] = SEC1_IDENTITY;
        return EncodedPoint {
            buf: b,
            len: 1,
            coord_size: 0,
        };
    }

    /// Construct an uncompressed SEC1 point (`0x04 || X || Y`).
    pub static fn fromUncompressed(x: []const u8, y: []const u8): PointResult {
        if (x.len == 0 || x.len != y.len || (1 as usize) + x.len + y.len > SEC1_MAX_POINT_LEN) {
            return PointResult { ok: false, point: EncodedPoint.identity(), err: SEC1_ERR_INVALID_POINT_LEN };
        }
        let b = sec1ZeroPointBuf();
        b[0] = SEC1_UNCOMPRESSED;
        let i: usize = 0;
        while (i < x.len) {
            b[1 + i] = x[i];
            b[1 + x.len + i] = y[i];
            i = i + 1;
        }
        return PointResult {
            ok: true,
            point: EncodedPoint {
                buf: b,
                len: (1 as usize) + x.len + y.len,
                coord_size: x.len,
            },
            err: SEC1_OK,
        };
    }

    /// Construct a compressed SEC1 point (`0x02 || X` if even Y, `0x03 || X` if odd Y).
    pub static fn fromCompressed(x: []const u8, y_is_odd: bool): PointResult {
        if (x.len == 0 || (1 as usize) + x.len > SEC1_MAX_POINT_LEN) {
            return PointResult { ok: false, point: EncodedPoint.identity(), err: SEC1_ERR_INVALID_POINT_LEN };
        }
        let b = sec1ZeroPointBuf();
        if (y_is_odd) {
            b[0] = SEC1_COMPRESSED_ODD;
        } else {
            b[0] = SEC1_COMPRESSED_EVEN;
        }
        let i: usize = 0;
        while (i < x.len) {
            b[1 + i] = x[i];
            i = i + 1;
        }
        return PointResult {
            ok: true,
            point: EncodedPoint {
                buf: b,
                len: (1 as usize) + x.len,
                coord_size: x.len,
            },
            err: SEC1_OK,
        };
    }

    /// Parse a SEC1 v2.0 encoded point from bytes (`0x00`, `0x02||X`, `0x03||X`, or `0x04||X||Y`).
    pub static fn fromBytes(bytes: []const u8): PointResult {
        if (bytes.len == 0 || bytes.len > SEC1_MAX_POINT_LEN) {
            return PointResult { ok: false, point: EncodedPoint.identity(), err: SEC1_ERR_INVALID_POINT_LEN };
        }
        const t = bytes[0];
        let csize: usize = 0;
        if (t == SEC1_IDENTITY) {
            if (bytes.len != 1) {
                return PointResult { ok: false, point: EncodedPoint.identity(), err: SEC1_ERR_INVALID_POINT_LEN };
            }
            return PointResult { ok: true, point: EncodedPoint.identity(), err: SEC1_OK };
        } else if (t == SEC1_COMPRESSED_EVEN || t == SEC1_COMPRESSED_ODD) {
            if (bytes.len < 2) {
                return PointResult { ok: false, point: EncodedPoint.identity(), err: SEC1_ERR_INVALID_POINT_LEN };
            }
            csize = bytes.len - (1 as usize);
        } else if (t == SEC1_UNCOMPRESSED) {
            if (bytes.len < 3 || ((bytes.len - (1 as usize)) % 2) != 0) {
                return PointResult { ok: false, point: EncodedPoint.identity(), err: SEC1_ERR_INVALID_POINT_LEN };
            }
            csize = (bytes.len - (1 as usize)) / (2 as usize);
        } else {
            return PointResult { ok: false, point: EncodedPoint.identity(), err: SEC1_ERR_INVALID_POINT_TAG };
        }

        let b = sec1ZeroPointBuf();
        let i: usize = 0;
        while (i < bytes.len) {
            b[i] = bytes[i];
            i = i + 1;
        }
        return PointResult {
            ok: true,
            point: EncodedPoint {
                buf: b,
                len: bytes.len,
                coord_size: csize,
            },
            err: SEC1_OK,
        };
    }

    /// Parse a SEC1 encoded point and verify its coordinate size matches `expected_coord_size`
    /// (unless it is the identity point `0x00`).
    pub static fn fromBytesForCoord(bytes: []const u8, expected_coord_size: usize): PointResult {
        const res = EncodedPoint.fromBytes(bytes);
        if (!res.ok) {
            return res;
        }
        if (!res.point.isIdentity() && res.point.coord_size != expected_coord_size) {
            return PointResult { ok: false, point: EncodedPoint.identity(), err: SEC1_ERR_INVALID_POINT_LEN };
        }
        return res;
    }

    pub fn tag(): u8 {
        if (_.len == 0) {
            return 0 as u8;
        }
        return _.buf[0];
    }

    pub fn isIdentity(): bool {
        return _.len == 1 && _.buf[0] == SEC1_IDENTITY;
    }

    pub fn isCompressed(): bool {
        return _.len > 1 && (_.buf[0] == SEC1_COMPRESSED_EVEN || _.buf[0] == SEC1_COMPRESSED_ODD);
    }

    pub fn isUncompressed(): bool {
        return _.len > 2 && _.buf[0] == SEC1_UNCOMPRESSED;
    }

    /// Copy X coordinate bytes into `out`. Returns `coord_size`, or 0 for identity / small buffer.
    pub fn xBytes(out: []u8): usize {
        if (_.isIdentity() || _.coord_size == 0 || out.len < _.coord_size) {
            return 0;
        }
        let i: usize = 0;
        while (i < _.coord_size) {
            out[i] = _.buf[1 + i];
            i = i + 1;
        }
        return _.coord_size;
    }

    /// Copy Y coordinate bytes into `out` (uncompressed points only). Returns `coord_size`, or 0 otherwise.
    pub fn yBytes(out: []u8): usize {
        if (!_.isUncompressed() || _.coord_size == 0 || out.len < _.coord_size) {
            return 0;
        }
        const y_off = (1 as usize) + _.coord_size;
        let i: usize = 0;
        while (i < _.coord_size) {
            out[i] = _.buf[y_off + i];
            i = i + 1;
        }
        return _.coord_size;
    }

    /// Copy the full SEC1 byte representation into `out`. Returns `len`, or 0 if buffer is too small.
    pub fn toBytes(out: []u8): usize {
        if (_.len == 0 || out.len < _.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < _.len) {
            out[i] = _.buf[i];
            i = i + 1;
        }
        return _.len;
    }

    /// Compress an uncompressed point by extracting Y's parity bit into tag `0x02`/`0x03`.
    /// Already-compressed and identity points are returned unchanged.
    pub fn compress(): PointResult {
        if (_.isIdentity() || _.isCompressed()) {
            return PointResult { ok: true, point: _, err: SEC1_OK };
        }
        if (!_.isUncompressed()) {
            return PointResult { ok: false, point: EncodedPoint.identity(), err: SEC1_ERR_INVALID_POINT_TAG };
        }
        const y_last = _.buf[(1 as usize) + _.coord_size + _.coord_size - (1 as usize)];
        const y_is_odd = (y_last & (1 as u8)) != (0 as u8);
        return EncodedPoint.fromCompressed(_.buf[1..((1 as usize) + _.coord_size)], y_is_odd);
    }

    pub fn eq(other: EncodedPoint): bool {
        if (_.len != other.len || _.coord_size != other.coord_size) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (_.buf[i] != other.buf[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }
}

pub struct EcPrivateKey {
    pub priv_key: [66]u8,
    pub priv_len: usize,
    pub has_curve: bool,
    pub curve_oid: ObjectIdentifier,
    pub has_pub_key: bool,
    pub pub_key: EncodedPoint,

    pub static fn empty(): EcPrivateKey {
        return EcPrivateKey {
            priv_key: sec1ZeroPrivBuf(),
            priv_len: 0,
            has_curve: false,
            curve_oid: ObjectIdentifier.empty(),
            has_pub_key: false,
            pub_key: EncodedPoint.identity(),
        };
    }

    /// Securely zeroize private key material via `@cap/subtle-ct`.
    pub fn zeroize(*_) {
        zeroizeBytes(_.priv_key[0..66]);
        _.priv_len = 0;
        zeroizeBytes(_.pub_key.buf[0..133]);
        _.pub_key.len = 0;
        _.pub_key.coord_size = 0;
        _.has_pub_key = false;
    }
}

pub struct EcPrivateKeyResult {
    pub ok: bool,
    pub key: EcPrivateKey,
    pub err: i64,
}

/// Construct an `EcPrivateKey` from raw private scalar bytes, optional curve OID, and optional public point.
pub fn makeEcPrivateKey(
    priv_bytes: []const u8,
    has_curve: bool,
    curve_oid: ObjectIdentifier,
    has_pub_key: bool,
    pub_key: EncodedPoint
): EcPrivateKeyResult {
    if (priv_bytes.len == 0 || priv_bytes.len > SEC1_MAX_PRIV_LEN) {
        return EcPrivateKeyResult { ok: false, key: EcPrivateKey.empty(), err: SEC1_ERR_INVALID_PRIV_KEY };
    }
    let pk = sec1ZeroPrivBuf();
    let i: usize = 0;
    while (i < priv_bytes.len) {
        pk[i] = priv_bytes[i];
        i = i + 1;
    }
    return EcPrivateKeyResult {
        ok: true,
        key: EcPrivateKey {
            priv_key: pk,
            priv_len: priv_bytes.len,
            has_curve: has_curve,
            curve_oid: curve_oid,
            has_pub_key: has_pub_key,
            pub_key: pub_key,
        },
        err: SEC1_OK,
    };
}

/// Encode an RFC 5915 `ECPrivateKey` SEQUENCE into DER bytes.
/// Structure: `SEQUENCE { INTEGER(1), OCTET STRING(priv), [0] EXPLICIT OID?, [1] EXPLICIT BIT STRING? }`.
pub fn encodeEcPrivateKeyDer(key: EcPrivateKey, out: []u8): i64 {
    if (key.priv_len == 0 || key.priv_len > SEC1_MAX_PRIV_LEN) {
        return SEC1_ERR_INVALID_PRIV_KEY;
    }

    // 1. version = INTEGER 1 -> 02 01 01 (3 bytes)
    const ver_len: usize = 3;

    // 2. privateKey = OCTET STRING
    const priv_tlv_len = (1 as usize) + sec1DerLenSize(key.priv_len) + key.priv_len;

    // 3. Optional [0] EXPLICIT curve OID
    let oid_tlv: [84]u8 = undefined;
    let oid_tlv_len: usize = 0;
    let ctx0_tlv_len: usize = 0;
    if (key.has_curve) {
        oid_tlv_len = key.curve_oid.toDerTlv(oid_tlv[0..84]);
        if (oid_tlv_len == 0) {
            return SEC1_ERR_INVALID_CURVE_OID;
        }
        ctx0_tlv_len = (1 as usize) + sec1DerLenSize(oid_tlv_len) + oid_tlv_len;
    }

    // 4. Optional [1] EXPLICIT BIT STRING (00 || pub_key.buf[0..pub_key.len])
    let bs_content_len: usize = 0;
    let bs_tlv_len: usize = 0;
    let ctx1_tlv_len: usize = 0;
    if (key.has_pub_key) {
        if (key.pub_key.len == 0 || key.pub_key.len > SEC1_MAX_POINT_LEN) {
            return SEC1_ERR_INVALID_PUB_KEY;
        }
        bs_content_len = (1 as usize) + key.pub_key.len;
        bs_tlv_len = (1 as usize) + sec1DerLenSize(bs_content_len) + bs_content_len;
        ctx1_tlv_len = (1 as usize) + sec1DerLenSize(bs_tlv_len) + bs_tlv_len;
    }

    const seq_content_len = ver_len + priv_tlv_len + ctx0_tlv_len + ctx1_tlv_len;
    const total_len = (1 as usize) + sec1DerLenSize(seq_content_len) + seq_content_len;
    if (out.len < total_len) {
        return SEC1_ERR_BUFFER_TOO_SMALL;
    }

    let pos: usize = 0;
    out[pos] = SEC1_TAG_SEQUENCE;
    pos = pos + 1;
    pos = sec1WriteDerLen(out, pos, seq_content_len);

    // version = 1
    out[pos] = SEC1_TAG_INTEGER;
    out[pos + 1] = 1 as u8;
    out[pos + 2] = 1 as u8;
    pos = pos + 3;

    // privateKey OCTET STRING
    out[pos] = SEC1_TAG_OCTET_STRING;
    pos = pos + 1;
    pos = sec1WriteDerLen(out, pos, key.priv_len);
    let i: usize = 0;
    while (i < key.priv_len) {
        out[pos + i] = key.priv_key[i];
        i = i + 1;
    }
    pos = pos + key.priv_len;

    // [0] EXPLICIT curve OID
    if (key.has_curve) {
        out[pos] = SEC1_TAG_CTX0_EXPLICIT;
        pos = pos + 1;
        pos = sec1WriteDerLen(out, pos, oid_tlv_len);
        i = 0;
        while (i < oid_tlv_len) {
            out[pos + i] = oid_tlv[i];
            i = i + 1;
        }
        pos = pos + oid_tlv_len;
    }

    // [1] EXPLICIT BIT STRING
    if (key.has_pub_key) {
        out[pos] = SEC1_TAG_CTX1_EXPLICIT;
        pos = pos + 1;
        pos = sec1WriteDerLen(out, pos, bs_tlv_len);
        out[pos] = SEC1_TAG_BIT_STRING;
        pos = pos + 1;
        pos = sec1WriteDerLen(out, pos, bs_content_len);
        out[pos] = 0 as u8; // unused_bits = 0
        pos = pos + 1;
        i = 0;
        while (i < key.pub_key.len) {
            out[pos + i] = key.pub_key.buf[i];
            i = i + 1;
        }
        pos = pos + key.pub_key.len;
    }

    return pos as i64;
}

/// Decode a DER-encoded RFC 5915 `ECPrivateKey` SEQUENCE.
/// Strictly requires version == 1, canonical DER lengths, valid curve OID, and valid SEC1 public point.
pub fn decodeEcPrivateKeyDer(der: []const u8): EcPrivateKeyResult {
    const empty_key = EcPrivateKey.empty();
    if (der.len < 6 || der[0] != SEC1_TAG_SEQUENCE) {
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_DER };
    }

    const seq_l = sec1ReadDerLen(der, 1);
    if (!seq_l.ok) {
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_DER };
    }
    const seq_end = seq_l.next_pos + seq_l.val_len;
    if (seq_end != der.len) {
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_TRAILING_DATA };
    }

    let pos = seq_l.next_pos;

    // 1. version INTEGER must be exactly 02 01 01
    if (pos + 3 > seq_end || der[pos] != SEC1_TAG_INTEGER || der[pos + 1] != (1 as u8)) {
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_VERSION };
    }
    if (der[pos + 2] != (1 as u8)) {
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_VERSION };
    }
    pos = pos + 3;

    // 2. privateKey OCTET STRING
    if (pos >= seq_end || der[pos] != SEC1_TAG_OCTET_STRING) {
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PRIV_KEY };
    }
    const priv_l = sec1ReadDerLen(der, pos + 1);
    if (!priv_l.ok || priv_l.val_len == 0 || priv_l.val_len > SEC1_MAX_PRIV_LEN) {
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PRIV_KEY };
    }
    if (priv_l.next_pos + priv_l.val_len > seq_end) {
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PRIV_KEY };
    }

    let pk = sec1ZeroPrivBuf();
    let i: usize = 0;
    while (i < priv_l.val_len) {
        pk[i] = der[priv_l.next_pos + i];
        i = i + 1;
    }
    pos = priv_l.next_pos + priv_l.val_len;

    // 3. Optional [0] EXPLICIT curve OID
    let has_curve = false;
    let curve_oid = ObjectIdentifier.empty();
    if (pos < seq_end && der[pos] == SEC1_TAG_CTX0_EXPLICIT) {
        const c0_l = sec1ReadDerLen(der, pos + 1);
        if (!c0_l.ok || c0_l.next_pos + c0_l.val_len > seq_end) {
            return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_CURVE_OID };
        }
        const c0_end = c0_l.next_pos + c0_l.val_len;
        if (c0_l.next_pos >= c0_end || der[c0_l.next_pos] != OID_TAG) {
            return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_CURVE_OID };
        }
        const oid_res = ObjectIdentifier.fromDerTlv(der[c0_l.next_pos..c0_end]);
        if (!oid_res.ok) {
            return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_CURVE_OID };
        }
        has_curve = true;
        curve_oid = oid_res.oid;
        pos = c0_end;
    }

    // 4. Optional [1] EXPLICIT BIT STRING public key
    let has_pub_key = false;
    let pub_key = EncodedPoint.identity();
    if (pos < seq_end && der[pos] == SEC1_TAG_CTX1_EXPLICIT) {
        const c1_l = sec1ReadDerLen(der, pos + 1);
        if (!c1_l.ok || c1_l.next_pos + c1_l.val_len > seq_end) {
            return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PUB_KEY };
        }
        const c1_end = c1_l.next_pos + c1_l.val_len;
        if (c1_l.next_pos >= c1_end || der[c1_l.next_pos] != SEC1_TAG_BIT_STRING) {
            return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PUB_KEY };
        }
        const bs_l = sec1ReadDerLen(der, c1_l.next_pos + 1);
        if (!bs_l.ok || bs_l.val_len < 2 || bs_l.next_pos + bs_l.val_len != c1_end) {
            return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PUB_KEY };
        }
        if (der[bs_l.next_pos] != (0 as u8)) {
            return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PUB_KEY };
        }
        const pt_res = EncodedPoint.fromBytes(der[(bs_l.next_pos + 1)..c1_end]);
        if (!pt_res.ok) {
            return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PUB_KEY };
        }
        has_pub_key = true;
        pub_key = pt_res.point;
        pos = c1_end;
    }

    // Reject unknown extra fields inside SEQUENCE
    if (pos != seq_end) {
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_TRAILING_DATA };
    }

    return EcPrivateKeyResult {
        ok: true,
        key: EcPrivateKey {
            priv_key: pk,
            priv_len: priv_l.val_len,
            has_curve: has_curve,
            curve_oid: curve_oid,
            has_pub_key: has_pub_key,
            pub_key: pub_key,
        },
        err: SEC1_OK,
    };
}

/// Encode an `EcPrivateKey` into RFC 7468 PEM (`-----BEGIN EC PRIVATE KEY-----`).
/// Reuses `@cap/pem-rfc7468` and zeroizes the temporary DER buffer via `@cap/subtle-ct`.
pub fn encodeEcPrivateKeyPem(key: EcPrivateKey, crlf: bool, out: []u8): i64 {
    let der_buf: [384]u8 = undefined;
    const dlen = encodeEcPrivateKeyDer(key, der_buf[0..384]);
    if (dlen < 0) {
        zeroizeBytes(der_buf[0..384]);
        return dlen;
    }
    const plen = encodePem(SEC1_PEM_LABEL, der_buf[0..(dlen as usize)], crlf, out);
    zeroizeBytes(der_buf[0..384]);
    return plen;
}

/// Decode an RFC 7468 `EC PRIVATE KEY` PEM block into an `EcPrivateKey`.
/// Strictly verifies the PEM label is `"EC PRIVATE KEY"` and zeroizes the temporary DER buffer.
pub fn decodeEcPrivateKeyPem(pem: string): EcPrivateKeyResult {
    const empty_key = EcPrivateKey.empty();
    let label_buf: [64]u8 = undefined;
    let label_len: usize = 0;
    let der_buf: [384]u8 = undefined;

    const dlen = decodePem(pem, label_buf[0..64], &label_len, der_buf[0..384]);
    if (dlen < 0) {
        zeroizeBytes(der_buf[0..384]);
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PEM };
    }
    if (!sec1StrEqBytes(SEC1_PEM_LABEL, label_buf[0..label_len])) {
        zeroizeBytes(der_buf[0..384]);
        return EcPrivateKeyResult { ok: false, key: empty_key, err: SEC1_ERR_INVALID_PEM_LABEL };
    }

    const res = decodeEcPrivateKeyDer(der_buf[0..(dlen as usize)]);
    zeroizeBytes(der_buf[0..384]);
    return res;
}

export {
    SEC1_IDENTITY,
    SEC1_COMPRESSED_EVEN,
    SEC1_COMPRESSED_ODD,
    SEC1_UNCOMPRESSED,
    SEC1_MAX_POINT_LEN,
    SEC1_MAX_PRIV_LEN,
    SEC1_P256_COORD_LEN,
    SEC1_P384_COORD_LEN,
    SEC1_P521_COORD_LEN,
    SEC1_PEM_LABEL,
    SEC1_OID_PRIME256V1,
    SEC1_OID_SECP256K1,
    SEC1_OID_SECP384R1,
    SEC1_OID_SECP521R1,
    SEC1_OK,
    SEC1_ERR_BUFFER_TOO_SMALL,
    SEC1_ERR_INVALID_POINT_TAG,
    SEC1_ERR_INVALID_POINT_LEN,
    SEC1_ERR_INVALID_DER,
    SEC1_ERR_INVALID_VERSION,
    SEC1_ERR_INVALID_PRIV_KEY,
    SEC1_ERR_INVALID_CURVE_OID,
    SEC1_ERR_INVALID_PUB_KEY,
    SEC1_ERR_INVALID_PEM,
    SEC1_ERR_INVALID_PEM_LABEL,
    SEC1_ERR_TRAILING_DATA,
    PointResult,
    EncodedPoint,
    EcPrivateKey,
    EcPrivateKeyResult,
    makeEcPrivateKey,
    encodeEcPrivateKeyDer,
    decodeEcPrivateKeyDer,
    encodeEcPrivateKeyPem,
    decodeEcPrivateKeyPem,
};
