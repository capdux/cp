// @cap/spki — Pure CP zero-allocation RFC 5280 SubjectPublicKeyInfo & AlgorithmIdentifier
// Clean-room MIT implementation with parity to RustCrypto `spki`.
// Reuses:
//   - `ObjectIdentifier`, standard OIDs, and DER OID codec from `@cap/const-oid`
//   - `encodePem`, `decodePem` from `@cap/pem-rfc7468` (RFC 7468 PUBLIC KEY PEM)

import {
    OID_TAG,
    OID_RSA_ENCRYPTION,
    OID_EC_PUBLIC_KEY,
    OID_PRIME256V1,
    OID_ED25519,
    ObjectIdentifier,
} from "../../const-oid/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";

pub const SPKI_MAX_KEY_LEN: usize = 160;
pub const SPKI_PEM_LABEL: string = "PUBLIC KEY";

pub const SPKI_TAG_BIT_STRING: u8 = 0x03;
pub const SPKI_TAG_NULL: u8 = 0x05;
pub const SPKI_TAG_SEQUENCE: u8 = 0x30;

pub const SPKI_OK: i64 = 0;
pub const SPKI_ERR_BUFFER_TOO_SMALL: i64 = -1;
pub const SPKI_ERR_INVALID_OID: i64 = -2;
pub const SPKI_ERR_INVALID_DER: i64 = -3;
pub const SPKI_ERR_INVALID_TAG: i64 = -4;
pub const SPKI_ERR_TRUNCATED: i64 = -5;
pub const SPKI_ERR_NON_ZERO_UNUSED_BITS: i64 = -6;
pub const SPKI_ERR_KEY_TOO_LONG: i64 = -7;
pub const SPKI_ERR_INVALID_PARAMS: i64 = -8;
pub const SPKI_ERR_INVALID_PEM: i64 = -9;
pub const SPKI_ERR_INVALID_PEM_LABEL: i64 = -10;
pub const SPKI_ERR_TRAILING_DATA: i64 = -11;

struct SpkiLenDecode {
    ok: bool,
    val_len: usize,
    next_pos: usize,
}

fn spkiZeroKeyBytes(): [160]u8 {
    let b: [160]u8 = undefined;
    let i: usize = 0;
    while (i < 160) {
        b[i] = 0 as u8;
        i = i + 1;
    }
    return b;
}

fn spkiDerLenSize(len: usize): usize {
    if (len < 128) {
        return 1;
    }
    if (len < 256) {
        return 2;
    }
    return 3;
}

fn spkiWriteDerLen(out: []u8, pos: usize, len: usize): usize {
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

fn spkiReadDerLen(der: []const u8, pos: usize): SpkiLenDecode {
    if (pos >= der.len) {
        return SpkiLenDecode { ok: false, val_len: 0, next_pos: pos };
    }
    const b0 = der[pos];
    if ((b0 & (0x80 as u8)) == 0) {
        const vlen = b0 as usize;
        const npos = pos + 1;
        if (npos + vlen > der.len) {
            return SpkiLenDecode { ok: false, val_len: 0, next_pos: pos };
        }
        return SpkiLenDecode { ok: true, val_len: vlen, next_pos: npos };
    }

    const nbytes = (b0 & (0x7F as u8)) as usize;
    if (nbytes == 0 || nbytes > 2) {
        return SpkiLenDecode { ok: false, val_len: 0, next_pos: pos };
    }
    if (pos + 1 + nbytes > der.len) {
        return SpkiLenDecode { ok: false, val_len: 0, next_pos: pos };
    }

    let vlen: usize = 0;
    if (nbytes == 1) {
        vlen = der[pos + 1] as usize;
        // Canonical DER forbids long form for lengths < 128
        if (vlen < 128) {
            return SpkiLenDecode { ok: false, val_len: 0, next_pos: pos };
        }
    } else {
        vlen = ((der[pos + 1] as usize) << (8 as u6)) | (der[pos + 2] as usize);
        // Canonical DER forbids leading zero byte in 2-byte length
        if (vlen < 256) {
            return SpkiLenDecode { ok: false, val_len: 0, next_pos: pos };
        }
    }

    const npos = pos + 1 + nbytes;
    if (npos + vlen > der.len) {
        return SpkiLenDecode { ok: false, val_len: 0, next_pos: pos };
    }
    return SpkiLenDecode { ok: true, val_len: vlen, next_pos: npos };
}

fn spkiStrEqBytes(s: string, b: []const u8): bool {
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

pub struct AlgorithmIdentifier {
    pub oid: ObjectIdentifier,
    pub has_params: bool,
    pub param_is_null: bool,
    pub param_oid: ObjectIdentifier,
    pub has_param_oid: bool,

    pub static fn empty(): AlgorithmIdentifier {
        return AlgorithmIdentifier {
            oid: ObjectIdentifier.empty(),
            has_params: false,
            param_is_null: false,
            param_oid: ObjectIdentifier.empty(),
            has_param_oid: false,
        };
    }

    /// AlgorithmIdentifier without parameters (e.g. Ed25519, X25519).
    pub static fn fromOid(oid: ObjectIdentifier): AlgorithmIdentifier {
        return AlgorithmIdentifier {
            oid: oid,
            has_params: false,
            param_is_null: false,
            param_oid: ObjectIdentifier.empty(),
            has_param_oid: false,
        };
    }

    /// AlgorithmIdentifier with ASN.1 NULL parameters (e.g. rsaEncryption).
    pub static fn withNull(oid: ObjectIdentifier): AlgorithmIdentifier {
        return AlgorithmIdentifier {
            oid: oid,
            has_params: true,
            param_is_null: true,
            param_oid: ObjectIdentifier.empty(),
            has_param_oid: false,
        };
    }

    /// AlgorithmIdentifier with OID parameters (e.g. ecPublicKey + prime256v1).
    pub static fn withParamOid(oid: ObjectIdentifier, param_oid: ObjectIdentifier): AlgorithmIdentifier {
        return AlgorithmIdentifier {
            oid: oid,
            has_params: true,
            param_is_null: false,
            param_oid: param_oid,
            has_param_oid: true,
        };
    }

    pub fn eq(other: AlgorithmIdentifier): bool {
        if (!_.oid.eq(other.oid)) {
            return false;
        }
        if (_.has_params != other.has_params) {
            return false;
        }
        if (_.param_is_null != other.param_is_null) {
            return false;
        }
        if (_.has_param_oid != other.has_param_oid) {
            return false;
        }
        if (_.has_param_oid && !_.param_oid.eq(other.param_oid)) {
            return false;
        }
        return true;
    }
}

pub struct AlgIdDecodeResult {
    pub ok: bool,
    pub alg: AlgorithmIdentifier,
    pub consumed: usize,
    pub err: i64,
}

pub struct SubjectPublicKeyInfo {
    pub algorithm: AlgorithmIdentifier,
    pub key_bytes: [160]u8,
    pub key_len: usize,
    pub unused_bits: u8,

    pub static fn empty(): SubjectPublicKeyInfo {
        return SubjectPublicKeyInfo {
            algorithm: AlgorithmIdentifier.empty(),
            key_bytes: spkiZeroKeyBytes(),
            key_len: 0,
            unused_bits: 0 as u8,
        };
    }

    /// Construct an RFC 8410 Ed25519 SubjectPublicKeyInfo from a 32-byte raw public key.
    pub static fn ed25519(pub_key: []const u8): SpkiDecodeResult {
        if (pub_key.len != 32) {
            return SpkiDecodeResult {
                ok: false,
                spki: SubjectPublicKeyInfo.empty(),
                err: SPKI_ERR_KEY_TOO_LONG,
            };
        }
        const oid_res = ObjectIdentifier.fromStr(OID_ED25519);
        if (!oid_res.ok) {
            return SpkiDecodeResult {
                ok: false,
                spki: SubjectPublicKeyInfo.empty(),
                err: SPKI_ERR_INVALID_OID,
            };
        }
        let kb = spkiZeroKeyBytes();
        let i: usize = 0;
        while (i < 32) {
            kb[i] = pub_key[i];
            i = i + 1;
        }
        return SpkiDecodeResult {
            ok: true,
            spki: SubjectPublicKeyInfo {
                algorithm: AlgorithmIdentifier.fromOid(oid_res.oid),
                key_bytes: kb,
                key_len: 32,
                unused_bits: 0 as u8,
            },
            err: SPKI_OK,
        };
    }

    /// Construct an RFC 5480 NIST P-256 (prime256v1) ECDSA SubjectPublicKeyInfo
    /// from an uncompressed (65-byte) or compressed (33-byte) SEC1 point.
    pub static fn p256(pub_key: []const u8): SpkiDecodeResult {
        if (pub_key.len != 65 && pub_key.len != 33) {
            return SpkiDecodeResult {
                ok: false,
                spki: SubjectPublicKeyInfo.empty(),
                err: SPKI_ERR_KEY_TOO_LONG,
            };
        }
        const alg_oid = ObjectIdentifier.fromStr(OID_EC_PUBLIC_KEY);
        const curve_oid = ObjectIdentifier.fromStr(OID_PRIME256V1);
        if (!alg_oid.ok || !curve_oid.ok) {
            return SpkiDecodeResult {
                ok: false,
                spki: SubjectPublicKeyInfo.empty(),
                err: SPKI_ERR_INVALID_OID,
            };
        }
        let kb = spkiZeroKeyBytes();
        let i: usize = 0;
        while (i < pub_key.len) {
            kb[i] = pub_key[i];
            i = i + 1;
        }
        return SpkiDecodeResult {
            ok: true,
            spki: SubjectPublicKeyInfo {
                algorithm: AlgorithmIdentifier.withParamOid(alg_oid.oid, curve_oid.oid),
                key_bytes: kb,
                key_len: pub_key.len,
                unused_bits: 0 as u8,
            },
            err: SPKI_OK,
        };
    }

    /// Construct an RSA SubjectPublicKeyInfo with NULL algorithm parameters.
    pub static fn rsa(pub_key_der: []const u8): SpkiDecodeResult {
        if (pub_key_der.len == 0 || pub_key_der.len > SPKI_MAX_KEY_LEN) {
            return SpkiDecodeResult {
                ok: false,
                spki: SubjectPublicKeyInfo.empty(),
                err: SPKI_ERR_KEY_TOO_LONG,
            };
        }
        const rsa_oid = ObjectIdentifier.fromStr(OID_RSA_ENCRYPTION);
        if (!rsa_oid.ok) {
            return SpkiDecodeResult {
                ok: false,
                spki: SubjectPublicKeyInfo.empty(),
                err: SPKI_ERR_INVALID_OID,
            };
        }
        let kb = spkiZeroKeyBytes();
        let i: usize = 0;
        while (i < pub_key_der.len) {
            kb[i] = pub_key_der[i];
            i = i + 1;
        }
        return SpkiDecodeResult {
            ok: true,
            spki: SubjectPublicKeyInfo {
                algorithm: AlgorithmIdentifier.withNull(rsa_oid.oid),
                key_bytes: kb,
                key_len: pub_key_der.len,
                unused_bits: 0 as u8,
            },
            err: SPKI_OK,
        };
    }
}

pub struct SpkiDecodeResult {
    pub ok: bool,
    pub spki: SubjectPublicKeyInfo,
    pub err: i64,
}

/// Encode an RFC 5280 `AlgorithmIdentifier` SEQUENCE into DER bytes.
/// Returns bytes written on success, or a negative `SPKI_ERR_*` code.
pub fn encodeAlgorithmId(alg: AlgorithmIdentifier, out: []u8): i64 {
    let oid_tlv: [84]u8 = undefined;
    const oid_len: usize = alg.oid.toDerTlv(oid_tlv[0..84]);
    if (oid_len == 0) {
        return SPKI_ERR_INVALID_OID;
    }

    let param_tlv: [84]u8 = undefined;
    let param_len: usize = 0;
    if (alg.has_params) {
        if (alg.param_is_null) {
            param_tlv[0] = SPKI_TAG_NULL;
            param_tlv[1] = 0 as u8;
            param_len = 2;
        } else if (alg.has_param_oid) {
            param_len = alg.param_oid.toDerTlv(param_tlv[0..84]);
            if (param_len == 0) {
                return SPKI_ERR_INVALID_PARAMS;
            }
        } else {
            return SPKI_ERR_INVALID_PARAMS;
        }
    }

    const content_len = oid_len + param_len;
    const total_len = (1 as usize) + spkiDerLenSize(content_len) + content_len;
    if (out.len < total_len) {
        return SPKI_ERR_BUFFER_TOO_SMALL;
    }

    let pos: usize = 0;
    out[pos] = SPKI_TAG_SEQUENCE;
    pos = pos + 1;
    pos = spkiWriteDerLen(out, pos, content_len);

    let i: usize = 0;
    while (i < oid_len) {
        out[pos + i] = oid_tlv[i];
        i = i + 1;
    }
    pos = pos + oid_len;

    i = 0;
    while (i < param_len) {
        out[pos + i] = param_tlv[i];
        i = i + 1;
    }
    pos = pos + param_len;

    return pos as i64;
}

fn spkiDecodeAlgIdSlice(der: []const u8, start_pos: usize): AlgIdDecodeResult {
    const empty_alg = AlgorithmIdentifier.empty();
    if (start_pos >= der.len) {
        return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_TRUNCATED };
    }
    if (der[start_pos] != SPKI_TAG_SEQUENCE) {
        return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_INVALID_TAG };
    }

    const seq_len = spkiReadDerLen(der, start_pos + 1);
    if (!seq_len.ok) {
        return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_INVALID_DER };
    }

    const seq_end = seq_len.next_pos + seq_len.val_len;
    let pos = seq_len.next_pos;

    // First element must be OBJECT IDENTIFIER (0x06)
    if (pos >= seq_end || der[pos] != OID_TAG) {
        return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_INVALID_OID };
    }
    const oid_l = spkiReadDerLen(der, pos + 1);
    if (!oid_l.ok || oid_l.next_pos + oid_l.val_len > seq_end) {
        return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_INVALID_OID };
    }
    const oid_end = oid_l.next_pos + oid_l.val_len;
    const oid_res = ObjectIdentifier.fromDerTlv(der[pos..oid_end]);
    if (!oid_res.ok) {
        return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_INVALID_OID };
    }
    pos = oid_end;

    // Optional parameters: absent, NULL (05 00), or OID (06 ...)
    let has_params = false;
    let param_is_null = false;
    let has_param_oid = false;
    let param_oid = ObjectIdentifier.empty();

    if (pos < seq_end) {
        has_params = true;
        const ptag = der[pos];
        if (ptag == SPKI_TAG_NULL) {
            if (pos + 2 != seq_end || der[pos + 1] != (0 as u8)) {
                return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_INVALID_PARAMS };
            }
            param_is_null = true;
            pos = seq_end;
        } else if (ptag == OID_TAG) {
            const poid_l = spkiReadDerLen(der, pos + 1);
            if (!poid_l.ok || poid_l.next_pos + poid_l.val_len != seq_end) {
                return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_INVALID_PARAMS };
            }
            const poid_res = ObjectIdentifier.fromDerTlv(der[pos..seq_end]);
            if (!poid_res.ok) {
                return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_INVALID_PARAMS };
            }
            has_param_oid = true;
            param_oid = poid_res.oid;
            pos = seq_end;
        } else {
            return AlgIdDecodeResult { ok: false, alg: empty_alg, consumed: 0, err: SPKI_ERR_INVALID_PARAMS };
        }
    }

    return AlgIdDecodeResult {
        ok: true,
        alg: AlgorithmIdentifier {
            oid: oid_res.oid,
            has_params: has_params,
            param_is_null: param_is_null,
            param_oid: param_oid,
            has_param_oid: has_param_oid,
        },
        consumed: seq_end - start_pos,
        err: SPKI_OK,
    };
}

/// Decode a standalone DER-encoded `AlgorithmIdentifier` SEQUENCE.
/// Rejects trailing bytes beyond the SEQUENCE.
pub fn decodeAlgorithmId(der: []const u8): AlgIdDecodeResult {
    const res = spkiDecodeAlgIdSlice(der, 0);
    if (!res.ok) {
        return res;
    }
    if (res.consumed != der.len) {
        return AlgIdDecodeResult {
            ok: false,
            alg: AlgorithmIdentifier.empty(),
            consumed: 0,
            err: SPKI_ERR_TRAILING_DATA,
        };
    }
    return res;
}

/// Encode an RFC 5280 `SubjectPublicKeyInfo` SEQUENCE into DER bytes.
/// Returns bytes written on success, or a negative `SPKI_ERR_*` code.
pub fn encodeSpkiDer(spki: SubjectPublicKeyInfo, out: []u8): i64 {
    if (spki.key_len > SPKI_MAX_KEY_LEN) {
        return SPKI_ERR_KEY_TOO_LONG;
    }
    if (spki.unused_bits != (0 as u8)) {
        return SPKI_ERR_NON_ZERO_UNUSED_BITS;
    }

    let alg_buf: [192]u8 = undefined;
    const alg_res = encodeAlgorithmId(spki.algorithm, alg_buf[0..192]);
    if (alg_res < 0) {
        return alg_res;
    }
    const alg_len = alg_res as usize;

    // BIT STRING content = 1 byte unused_bits + key_len bytes
    const bs_content_len = (1 as usize) + spki.key_len;
    const bs_tlv_len = (1 as usize) + spkiDerLenSize(bs_content_len) + bs_content_len;

    const seq_content_len = alg_len + bs_tlv_len;
    const total_len = (1 as usize) + spkiDerLenSize(seq_content_len) + seq_content_len;
    if (out.len < total_len) {
        return SPKI_ERR_BUFFER_TOO_SMALL;
    }

    let pos: usize = 0;
    out[pos] = SPKI_TAG_SEQUENCE;
    pos = pos + 1;
    pos = spkiWriteDerLen(out, pos, seq_content_len);

    let i: usize = 0;
    while (i < alg_len) {
        out[pos + i] = alg_buf[i];
        i = i + 1;
    }
    pos = pos + alg_len;

    out[pos] = SPKI_TAG_BIT_STRING;
    pos = pos + 1;
    pos = spkiWriteDerLen(out, pos, bs_content_len);
    out[pos] = spki.unused_bits;
    pos = pos + 1;

    i = 0;
    while (i < spki.key_len) {
        out[pos + i] = spki.key_bytes[i];
        i = i + 1;
    }
    pos = pos + spki.key_len;

    return pos as i64;
}

/// Decode a DER-encoded RFC 5280 `SubjectPublicKeyInfo` SEQUENCE.
/// Strictly rejects malformed DER, truncated input, trailing data, and non-zero BIT STRING unused_bits.
pub fn decodeSpkiDer(der: []const u8): SpkiDecodeResult {
    const empty_spki = SubjectPublicKeyInfo.empty();
    if (der.len == 0) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_TRUNCATED };
    }
    if (der[0] != SPKI_TAG_SEQUENCE) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_INVALID_TAG };
    }

    const seq_len = spkiReadDerLen(der, 1);
    if (!seq_len.ok) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_INVALID_DER };
    }
    const seq_end = seq_len.next_pos + seq_len.val_len;
    if (seq_end != der.len) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_TRAILING_DATA };
    }

    let pos = seq_len.next_pos;
    const alg_res = spkiDecodeAlgIdSlice(der[0..seq_end], pos);
    if (!alg_res.ok) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: alg_res.err };
    }
    pos = pos + alg_res.consumed;

    // Second element must be BIT STRING (0x03)
    if (pos >= seq_end) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_TRUNCATED };
    }
    if (der[pos] != SPKI_TAG_BIT_STRING) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_INVALID_TAG };
    }
    const bs_len = spkiReadDerLen(der, pos + 1);
    if (!bs_len.ok) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_INVALID_DER };
    }
    if (bs_len.val_len < 1) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_INVALID_DER };
    }
    if (bs_len.next_pos + bs_len.val_len != seq_end) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_TRAILING_DATA };
    }

    const unused_bits = der[bs_len.next_pos];
    if (unused_bits != (0 as u8)) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_NON_ZERO_UNUSED_BITS };
    }

    const key_len = bs_len.val_len - (1 as usize);
    if (key_len > SPKI_MAX_KEY_LEN) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_KEY_TOO_LONG };
    }

    let kb = spkiZeroKeyBytes();
    let i: usize = 0;
    while (i < key_len) {
        kb[i] = der[bs_len.next_pos + 1 + i];
        i = i + 1;
    }

    return SpkiDecodeResult {
        ok: true,
        spki: SubjectPublicKeyInfo {
            algorithm: alg_res.alg,
            key_bytes: kb,
            key_len: key_len,
            unused_bits: unused_bits,
        },
        err: SPKI_OK,
    };
}

/// Encode a `SubjectPublicKeyInfo` into RFC 7468 PEM (`-----BEGIN PUBLIC KEY-----`).
/// Reuses `@cap/pem-rfc7468`. Returns bytes written on success, or a negative error code.
pub fn encodeSpkiPem(spki: SubjectPublicKeyInfo, crlf: bool, out: []u8): i64 {
    let der_buf: [384]u8 = undefined;
    const dlen = encodeSpkiDer(spki, der_buf[0..384]);
    if (dlen < 0) {
        return dlen;
    }
    return encodePem(SPKI_PEM_LABEL, der_buf[0..(dlen as usize)], crlf, out);
}

/// Decode an RFC 7468 PEM `PUBLIC KEY` block into a `SubjectPublicKeyInfo`.
/// Reuses `@cap/pem-rfc7468` and strictly verifies that the PEM label is `"PUBLIC KEY"`.
pub fn decodeSpkiPem(pem: string): SpkiDecodeResult {
    const empty_spki = SubjectPublicKeyInfo.empty();
    let label_buf: [64]u8 = undefined;
    let label_len: usize = 0;
    let der_buf: [384]u8 = undefined;

    const dlen = decodePem(pem, label_buf[0..64], &label_len, der_buf[0..384]);
    if (dlen < 0) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_INVALID_PEM };
    }
    if (!spkiStrEqBytes(SPKI_PEM_LABEL, label_buf[0..label_len])) {
        return SpkiDecodeResult { ok: false, spki: empty_spki, err: SPKI_ERR_INVALID_PEM_LABEL };
    }

    return decodeSpkiDer(der_buf[0..(dlen as usize)]);
}

/// Compute a fast deterministic 64-bit FNV-1a fingerprint over SPKI DER bytes.
pub fn fingerprintSpki(der: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325 as u64;
    const prime: u64 = 0x100000001b3 as u64;
    let i: usize = 0;
    while (i < der.len) {
        h = h ^ (der[i] as u64);
        // Wrapping 64-bit multiply via bit shifts (FNV prime = 2^40 + 2^8 + 0xb3)
        let lo = h & (0xFFFFFFFF as u64);
        let hi = h >> (32 as u6);
        let p_lo = prime & (0xFFFFFFFF as u64);
        let p_hi = prime >> (32 as u6);
        let r0 = lo * p_lo;
        let r1 = ((lo * p_hi) + (hi * p_lo) + (r0 >> (32 as u6))) & (0xFFFFFFFF as u64);
        h = (r0 & (0xFFFFFFFF as u64)) | (r1 << (32 as u6));
        i = i + 1;
    }
    return h;
}

export {
    SPKI_MAX_KEY_LEN,
    SPKI_PEM_LABEL,
    SPKI_TAG_BIT_STRING,
    SPKI_TAG_NULL,
    SPKI_TAG_SEQUENCE,
    SPKI_OK,
    SPKI_ERR_BUFFER_TOO_SMALL,
    SPKI_ERR_INVALID_OID,
    SPKI_ERR_INVALID_DER,
    SPKI_ERR_INVALID_TAG,
    SPKI_ERR_TRUNCATED,
    SPKI_ERR_NON_ZERO_UNUSED_BITS,
    SPKI_ERR_KEY_TOO_LONG,
    SPKI_ERR_INVALID_PARAMS,
    SPKI_ERR_INVALID_PEM,
    SPKI_ERR_INVALID_PEM_LABEL,
    SPKI_ERR_TRAILING_DATA,
    AlgorithmIdentifier,
    AlgIdDecodeResult,
    SubjectPublicKeyInfo,
    SpkiDecodeResult,
    encodeAlgorithmId,
    decodeAlgorithmId,
    encodeSpkiDer,
    decodeSpkiDer,
    encodeSpkiPem,
    decodeSpkiPem,
    fingerprintSpki,
};
