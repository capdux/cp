// @cap/ecdsa-sig — Pure CP zero-allocation ECDSA Signature Codec (RFC 3279 §2.2.3 & Low-S)
// Clean-room MIT implementation with parity to RustCrypto `ecdsa` / `signature`.
// Reuses:
//   - `DerReader`, `DerWriter`, `TAG_INTEGER` from `@cap/der-asn1`
//   - `ctEqBytes` from `@cap/subtle-ct` (constant-time signature comparison)

import { TAG_INTEGER, DerReader, DerWriter } from "../../der-asn1/src/index.cp";
import { ctEqBytes } from "../../subtle-ct/src/index.cp";

pub const ECDSA_P256_COORD_LEN: usize = 32;
pub const ECDSA_P384_COORD_LEN: usize = 48;
pub const ECDSA_P256_FIXED_LEN: usize = 64;
pub const ECDSA_P384_FIXED_LEN: usize = 96;
pub const ECDSA_MAX_COORD_LEN: usize = 48;

pub const ECDSA_OK: i64 = 0;
pub const ECDSA_ERR_INVALID_LEN: i64 = -1;
pub const ECDSA_ERR_ZERO_COMPONENT: i64 = -2;
pub const ECDSA_ERR_INVALID_DER: i64 = -3;
pub const ECDSA_ERR_NON_CANONICAL_INT: i64 = -4;
pub const ECDSA_ERR_NEGATIVE_INT: i64 = -5;
pub const ECDSA_ERR_INT_TOO_LARGE: i64 = -6;
pub const ECDSA_ERR_TRAILING_DATA: i64 = -7;
pub const ECDSA_ERR_BUFFER_TOO_SMALL: i64 = -8;

fn ecsigZero48(): [48]u8 {
    let b: [48]u8 = undefined;
    let i: usize = 0;
    while (i < 48) {
        b[i] = 0 as u8;
        i = i + 1;
    }
    return b;
}

fn ecsigIsAllZero(data: []const u8): bool {
    let acc: u8 = 0 as u8;
    let i: usize = 0;
    while (i < data.len) {
        acc = acc | data[i];
        i = i + 1;
    }
    return acc == (0 as u8);
}

fn ecsigP256Order(): [32]u8 {
    // NIST P-256 curve order n:
    // 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
    let n: [32]u8 = [
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
        0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51
    ];
    return n;
}

fn ecsigP256HalfOrder(): [32]u8 {
    // floor(n / 2):
    // 0x7FFFFFFF800000007FFFFFFFFFFFFFFFDE737D56D38BCF4279DCE5617E3192A8
    let h: [32]u8 = [
        0x7F, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00,
        0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xDE, 0x73, 0x7D, 0x56, 0xD3, 0x8B, 0xCF, 0x42,
        0x79, 0xDC, 0xE5, 0x61, 0x7E, 0x31, 0x92, 0xA8
    ];
    return h;
}

/// Big-endian 32-byte unsigned comparison: returns -1 if a < b, 0 if a == b, 1 if a > b.
fn ecsigCmpBe32(a: []const u8, b: []const u8): i32 {
    let i: usize = 0;
    while (i < 32) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
        i = i + 1;
    }
    return 0;
}

/// Write canonical minimal unsigned DER INTEGER into `out` via `DerWriter`.
fn ecsigWriteBeInt(w: *DerWriter, val: []const u8): bool {
    let start: usize = 0;
    while (start + 1 < val.len && val[start] == (0 as u8)) {
        start = start + 1;
    }
    const sig_len = val.len - start;
    let buf: [49]u8 = undefined;
    let pos: usize = 0;
    if ((val[start] & (0x80 as u8)) != (0 as u8)) {
        buf[0] = 0 as u8;
        pos = 1;
    }
    let i: usize = 0;
    while (i < sig_len) {
        buf[pos + i] = val[start + i];
        i = i + 1;
    }
    return w.writeTlv(TAG_INTEGER, buf[0..(pos + sig_len)]);
}

/// Read strict canonical non-negative DER INTEGER into right-aligned big-endian `out[0..coord_len]`.
fn ecsigReadBeInt(r: *DerReader, coord_len: usize, out: []u8): i64 {
    const tlv = r.readTlv();
    if (!tlv.ok || tlv.tag != TAG_INTEGER || tlv.value.len == 0) {
        return ECDSA_ERR_INVALID_DER;
    }
    const v = tlv.value;

    // Reject negative integers
    if ((v[0] & (0x80 as u8)) != (0 as u8)) {
        return ECDSA_ERR_NEGATIVE_INT;
    }

    // X.690 §8.3.2 minimal encoding: leading 0x00 is only legal if next byte has MSB set
    if (v.len > 1 && v[0] == (0 as u8) && (v[1] & (0x80 as u8)) == (0 as u8)) {
        return ECDSA_ERR_NON_CANONICAL_INT;
    }

    let start: usize = 0;
    if (v.len > 1 && v[0] == (0 as u8)) {
        start = 1;
    }
    const mag_len = v.len - start;
    if (mag_len > coord_len) {
        return ECDSA_ERR_INT_TOO_LARGE;
    }

    let i: usize = 0;
    while (i < coord_len) {
        out[i] = 0 as u8;
        i = i + 1;
    }
    const pad = coord_len - mag_len;
    i = 0;
    while (i < mag_len) {
        out[pad + i] = v[start + i];
        i = i + 1;
    }

    if (ecsigIsAllZero(out[0..coord_len])) {
        return ECDSA_ERR_ZERO_COMPONENT;
    }
    return ECDSA_OK;
}

pub struct EcdsaSigResult {
    pub ok: bool,
    pub sig: EcdsaSignature,
    pub err: i64,
}

/// Fixed-size zero-allocation ECDSA `(r, s)` signature supporting P-256 (32B) and P-384 (48B).
pub struct EcdsaSignature {
    pub r: [48]u8,
    pub s: [48]u8,
    pub coord_len: usize,

    pub static fn empty(): EcdsaSignature {
        return EcdsaSignature {
            r: ecsigZero48(),
            s: ecsigZero48(),
            coord_len: 0,
        };
    }

    /// Parse a fixed-width `r || s` ECDSA signature (64 bytes for P-256, 96 bytes for P-384).
    /// Rejects invalid lengths and all-zero `r` or `s` components.
    pub static fn fromFixed(bytes: []const u8): EcdsaSigResult {
        const empty_sig = EcdsaSignature.empty();
        let clen: usize = 0;
        if (bytes.len == ECDSA_P256_FIXED_LEN) {
            clen = ECDSA_P256_COORD_LEN;
        } else if (bytes.len == ECDSA_P384_FIXED_LEN) {
            clen = ECDSA_P384_COORD_LEN;
        } else {
            return EcdsaSigResult { ok: false, sig: empty_sig, err: ECDSA_ERR_INVALID_LEN };
        }

        if (ecsigIsAllZero(bytes[0..clen]) || ecsigIsAllZero(bytes[clen..(clen * 2)])) {
            return EcdsaSigResult { ok: false, sig: empty_sig, err: ECDSA_ERR_ZERO_COMPONENT };
        }

        let r_buf = ecsigZero48();
        let s_buf = ecsigZero48();
        let i: usize = 0;
        while (i < clen) {
            r_buf[i] = bytes[i];
            s_buf[i] = bytes[clen + i];
            i = i + 1;
        }

        return EcdsaSigResult {
            ok: true,
            sig: EcdsaSignature {
                r: r_buf,
                s: s_buf,
                coord_len: clen,
            },
            err: ECDSA_OK,
        };
    }

    /// Parse a strict RFC 3279 §2.2.3 ASN.1 DER `SEQUENCE { r INTEGER, s INTEGER }` signature.
    /// Requires `coord_len == 32` (P-256) or `48` (P-384).
    /// Rejects non-minimal integers, negative integers, oversized components, zero `r`/`s`, and trailing data.
    pub static fn fromDer(der: []const u8, coord_len: usize): EcdsaSigResult {
        const empty_sig = EcdsaSignature.empty();
        if (coord_len != ECDSA_P256_COORD_LEN && coord_len != ECDSA_P384_COORD_LEN) {
            return EcdsaSigResult { ok: false, sig: empty_sig, err: ECDSA_ERR_INVALID_LEN };
        }

        let top_r = DerReader.new(der);
        const seq = top_r.readSequence();
        if (!seq.ok) {
            return EcdsaSigResult { ok: false, sig: empty_sig, err: ECDSA_ERR_INVALID_DER };
        }
        if (!top_r.isEmpty()) {
            return EcdsaSigResult { ok: false, sig: empty_sig, err: ECDSA_ERR_TRAILING_DATA };
        }

        let inner_r = seq.reader;
        let r_buf = ecsigZero48();
        let s_buf = ecsigZero48();

        const err_r = ecsigReadBeInt(&inner_r, coord_len, r_buf[0..coord_len]);
        if (err_r != ECDSA_OK) {
            return EcdsaSigResult { ok: false, sig: empty_sig, err: err_r };
        }
        const err_s = ecsigReadBeInt(&inner_r, coord_len, s_buf[0..coord_len]);
        if (err_s != ECDSA_OK) {
            return EcdsaSigResult { ok: false, sig: empty_sig, err: err_s };
        }
        if (!inner_r.isEmpty()) {
            return EcdsaSigResult { ok: false, sig: empty_sig, err: ECDSA_ERR_TRAILING_DATA };
        }

        return EcdsaSigResult {
            ok: true,
            sig: EcdsaSignature {
                r: r_buf,
                s: s_buf,
                coord_len: coord_len,
            },
            err: ECDSA_OK,
        };
    }

    /// Encode signature as fixed-width `r || s` (64B for P-256, 96B for P-384).
    /// Returns bytes written, or 0 if invalid or `out` is too small.
    pub fn toFixed(out: []u8): usize {
        if (_.coord_len != ECDSA_P256_COORD_LEN && _.coord_len != ECDSA_P384_COORD_LEN) {
            return 0;
        }
        const total = _.coord_len * (2 as usize);
        if (out.len < total) {
            return 0;
        }
        let i: usize = 0;
        while (i < _.coord_len) {
            out[i] = _.r[i];
            out[_.coord_len + i] = _.s[i];
            i = i + 1;
        }
        return total;
    }

    /// Encode signature as canonical RFC 3279 §2.2.3 ASN.1 DER `SEQUENCE { INTEGER r, INTEGER s }`.
    /// Returns bytes written on success, or a negative `ECDSA_ERR_*` code.
    pub fn toDer(out: []u8): i64 {
        if (_.coord_len != ECDSA_P256_COORD_LEN && _.coord_len != ECDSA_P384_COORD_LEN) {
            return ECDSA_ERR_INVALID_LEN;
        }
        if (ecsigIsAllZero(_.r[0.._.coord_len]) || ecsigIsAllZero(_.s[0.._.coord_len])) {
            return ECDSA_ERR_ZERO_COMPONENT;
        }

        let inner_buf: [112]u8 = undefined;
        let iw = DerWriter.new(inner_buf[0..112]);
        if (!ecsigWriteBeInt(&iw, _.r[0.._.coord_len])) {
            return ECDSA_ERR_BUFFER_TOO_SMALL;
        }
        if (!ecsigWriteBeInt(&iw, _.s[0.._.coord_len])) {
            return ECDSA_ERR_BUFFER_TOO_SMALL;
        }

        let ow = DerWriter.new(out);
        if (!ow.writeSequence(iw.written())) {
            return ECDSA_ERR_BUFFER_TOO_SMALL;
        }
        return ow.len() as i64;
    }

    /// Return `true` if this is a P-256 signature with `0 < s <= floor(n / 2)` (BIP-62 Low-S).
    pub fn isLowSP256(): bool {
        if (_.coord_len != ECDSA_P256_COORD_LEN) {
            return false;
        }
        if (ecsigIsAllZero(_.s[0..32])) {
            return false;
        }
        let half_n = ecsigP256HalfOrder();
        return ecsigCmpBe32(_.s[0..32], half_n[0..32]) <= 0;
    }

    /// Normalize a high-S P-256 signature (`floor(n/2) < s < n`) in-place to `s = n - s`.
    /// Returns `true` if `s` was high-S and normalized, or `false` if already low-S or not P-256.
    pub fn normalizeLowSP256(*_): bool {
        if (_.coord_len != ECDSA_P256_COORD_LEN) {
            return false;
        }
        let half_n = ecsigP256HalfOrder();
        if (ecsigCmpBe32(_.s[0..32], half_n[0..32]) <= 0) {
            return false;
        }
        let order_n = ecsigP256Order();
        if (ecsigCmpBe32(_.s[0..32], order_n[0..32]) >= 0) {
            return false;
        }

        // Exact 256-bit big-endian subtraction: s = n - s
        let borrow: i32 = 0;
        let i: usize = 32;
        while (i > 0) {
            i = i - 1;
            let diff: i32 = (order_n[i] as i32) - (_.s[i] as i32) - borrow;
            if (diff < 0) {
                diff = diff + 256;
                borrow = 1;
            } else {
                borrow = 0;
            }
            _.s[i] = diff as u8;
        }
        return true;
    }

    /// Constant-time equality comparison via `@cap/subtle-ct`.
    pub fn ctEq(other: EcdsaSignature): bool {
        if (_.coord_len != other.coord_len) {
            return false;
        }
        if (_.coord_len != ECDSA_P256_COORD_LEN && _.coord_len != ECDSA_P384_COORD_LEN) {
            return false;
        }
        const r_eq = ctEqBytes(_.r[0.._.coord_len], other.r[0..other.coord_len]);
        const s_eq = ctEqBytes(_.s[0.._.coord_len], other.s[0..other.coord_len]);
        return r_eq.and(s_eq).toBool();
    }
}

export {
    ECDSA_P256_COORD_LEN,
    ECDSA_P384_COORD_LEN,
    ECDSA_P256_FIXED_LEN,
    ECDSA_P384_FIXED_LEN,
    ECDSA_MAX_COORD_LEN,
    ECDSA_OK,
    ECDSA_ERR_INVALID_LEN,
    ECDSA_ERR_ZERO_COMPONENT,
    ECDSA_ERR_INVALID_DER,
    ECDSA_ERR_NON_CANONICAL_INT,
    ECDSA_ERR_NEGATIVE_INT,
    ECDSA_ERR_INT_TOO_LARGE,
    ECDSA_ERR_TRAILING_DATA,
    ECDSA_ERR_BUFFER_TOO_SMALL,
    EcdsaSigResult,
    EcdsaSignature,
};
