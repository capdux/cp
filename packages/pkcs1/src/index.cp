// packages/@cap/pkcs1/src/index.cp
// Zero-allocation RFC 8017 Appendix A.1.1 (RSAPublicKey) & A.1.2 (RSAPrivateKey) DER/PEM codec.
// Reuses:
//   - DerReader, DerWriter, TAG_INTEGER from @cap/der-asn1
//   - encodePem, decodePem from @cap/pem-rfc7468
//   - zeroizeBytes from @cap/subtle-ct
// Clean-room MIT implementation (RustCrypto `pkcs1` parity).

import { TAG_INTEGER, DerReader, DerWriter } from "../../der/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";
import { zeroizeBytes } from "../../subtle/src/index.cp";

pub const PK1_MAX_MODULUS_LEN: usize = 512;
pub const PK1_MAX_PRIME_LEN: usize = 256;
pub const PK1_VERSION_TWO_PRIME: u8 = 0;

pub const PK1_PEM_PUBLIC_LABEL: string = "RSA PUBLIC KEY";
pub const PK1_PEM_PRIVATE_LABEL: string = "RSA PRIVATE KEY";

pub const PK1_OK: i64 = 0;
pub const PK1_ERR_BUFFER_TOO_SMALL: i64 = -1;
pub const PK1_ERR_INVALID_DER: i64 = -2;
pub const PK1_ERR_INVALID_VERSION: i64 = -3;
pub const PK1_ERR_INVALID_INTEGER: i64 = -4;
pub const PK1_ERR_NEGATIVE_INTEGER: i64 = -5;
pub const PK1_ERR_KEY_TOO_LONG: i64 = -6;
pub const PK1_ERR_TRAILING_DATA: i64 = -7;
pub const PK1_ERR_INVALID_PEM: i64 = -8;
pub const PK1_ERR_INVALID_PEM_LABEL: i64 = -9;

fn pk1StrEqBytes(s: string, b: []const u8): bool {
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

fn pk1WriteBigUint(w: *DerWriter, raw: []const u8): bool {
    // Strip leading zeros to obtain minimal unsigned magnitude
    let start: usize = 0;
    while (start < raw.len && raw[start] == (0 as u8)) {
        start = start + 1;
    }
    if (start == raw.len) {
        let z: [1]u8 = [0 as u8];
        return w.writeTlv(TAG_INTEGER, z[0..1]);
    }
    const mag_len = raw.len - start;
    if (mag_len > PK1_MAX_MODULUS_LEN) {
        return false;
    }
    // Prepend 0x00 if MSB has bit 7 set (sign-bit protection per X.690 §8.3)
    if ((raw[start] & (0x80 as u8)) != 0) {
        let tmp: [513]u8 = [0; 513];
        tmp[0] = 0 as u8;
        let i: usize = 0;
        while (i < mag_len) {
            tmp[1 + i] = raw[start + i];
            i = i + 1;
        }
        return w.writeTlv(TAG_INTEGER, tmp[0..(1 + mag_len)]);
    }
    return w.writeTlv(TAG_INTEGER, raw[start..raw.len]);
}

fn pk1ReadBigUint(r: *DerReader, dst: []u8, len_out: *usize): i64 {
    len_out.* = 0;
    const tlv = r.readTlv();
    if (!tlv.ok || tlv.tag != TAG_INTEGER || tlv.value.len == 0) {
        return PK1_ERR_INVALID_INTEGER;
    }
    const v = tlv.value;
    // Strict X.690 §8.3.2 minimal encoding check
    if (v.len > 1) {
        if (v[0] == (0x00 as u8) && (v[1] & (0x80 as u8)) == 0) {
            return PK1_ERR_INVALID_INTEGER;
        }
        if (v[0] == (0xFF as u8) && (v[1] & (0x80 as u8)) != 0) {
            return PK1_ERR_INVALID_INTEGER;
        }
    }
    // Reject negative integers
    if ((v[0] & (0x80 as u8)) != 0) {
        return PK1_ERR_NEGATIVE_INTEGER;
    }

    let start: usize = 0;
    if (v.len > 1 && v[0] == (0x00 as u8)) {
        start = 1;
    }
    const mag_len = v.len - start;
    if (mag_len > dst.len) {
        return PK1_ERR_KEY_TOO_LONG;
    }
    let i: usize = 0;
    while (i < mag_len) {
        dst[i] = v[start + i];
        i = i + 1;
    }
    len_out.* = mag_len;
    return PK1_OK;
}

/// RFC 8017 Appendix A.1.1 `RSAPublicKey` (unsigned big-endian modulus `n` + public exponent `e`).
pub struct RsaPublicKey {
    pub n: [512]u8,
    pub n_len: usize,
    pub e: u64,

    pub static fn empty(): RsaPublicKey {
        return RsaPublicKey {
            n: [0; 512],
            n_len: 0,
            e: 0 as u64,
        };
    }

    /// Constructs an `RsaPublicKey` from big-endian unsigned modulus bytes and exponent `e`.
    /// Strips leading zero bytes from `n_bytes`.
    pub static fn new(n_bytes: []const u8, e: u64): RsaPublicKeyResult {
        let start: usize = 0;
        while (start < n_bytes.len && n_bytes[start] == (0 as u8)) {
            start = start + 1;
        }
        if (start == n_bytes.len) {
            return RsaPublicKeyResult {
                ok: false,
                key: RsaPublicKey.empty(),
                err: PK1_ERR_INVALID_INTEGER,
            };
        }
        const mlen = n_bytes.len - start;
        if (mlen > PK1_MAX_MODULUS_LEN) {
            return RsaPublicKeyResult {
                ok: false,
                key: RsaPublicKey.empty(),
                err: PK1_ERR_KEY_TOO_LONG,
            };
        }
        let key = RsaPublicKey.empty();
        let i: usize = 0;
        while (i < mlen) {
            key.n[i] = n_bytes[start + i];
            i = i + 1;
        }
        key.n_len = mlen;
        key.e = e;
        return RsaPublicKeyResult { ok: true, key: key, err: PK1_OK };
    }
}

pub struct RsaPublicKeyResult {
    pub ok: bool,
    pub key: RsaPublicKey,
    pub err: i64,
}

/// RFC 8017 Appendix A.1.2 `RSAPrivateKey` (two-prime, 9 fields).
pub struct RsaPrivateKey {
    pub version: u8,
    pub n: [512]u8,
    pub n_len: usize,
    pub e: u64,
    pub d: [512]u8,
    pub d_len: usize,
    pub p: [256]u8,
    pub p_len: usize,
    pub q: [256]u8,
    pub q_len: usize,
    pub dp: [256]u8,
    pub dp_len: usize,
    pub dq: [256]u8,
    pub dq_len: usize,
    pub qinv: [256]u8,
    pub qinv_len: usize,

    pub static fn empty(): RsaPrivateKey {
        return RsaPrivateKey {
            version: PK1_VERSION_TWO_PRIME,
            n: [0; 512],
            n_len: 0,
            e: 0 as u64,
            d: [0; 512],
            d_len: 0,
            p: [0; 256],
            p_len: 0,
            q: [0; 256],
            q_len: 0,
            dp: [0; 256],
            dp_len: 0,
            dq: [0; 256],
            dq_len: 0,
            qinv: [0; 256],
            qinv_len: 0,
        };
    }

    /// Extracts the corresponding `RsaPublicKey` (`n`, `e`).
    pub fn publicKey(): RsaPublicKey {
        let pubk = RsaPublicKey.empty();
        let i: usize = 0;
        while (i < _.n_len && i < PK1_MAX_MODULUS_LEN) {
            pubk.n[i] = _.n[i];
            i = i + 1;
        }
        pubk.n_len = _.n_len;
        pubk.e = _.e;
        return pubk;
    }

    /// Securely wipes all private key material using `@cap/subtle-ct` (`zeroizeBytes`).
    pub fn zeroize(*_) {
        zeroizeBytes(_.n[0..PK1_MAX_MODULUS_LEN]);
        zeroizeBytes(_.d[0..PK1_MAX_MODULUS_LEN]);
        zeroizeBytes(_.p[0..PK1_MAX_PRIME_LEN]);
        zeroizeBytes(_.q[0..PK1_MAX_PRIME_LEN]);
        zeroizeBytes(_.dp[0..PK1_MAX_PRIME_LEN]);
        zeroizeBytes(_.dq[0..PK1_MAX_PRIME_LEN]);
        zeroizeBytes(_.qinv[0..PK1_MAX_PRIME_LEN]);
        _.n_len = 0;
        _.e = 0 as u64;
        _.d_len = 0;
        _.p_len = 0;
        _.q_len = 0;
        _.dp_len = 0;
        _.dq_len = 0;
        _.qinv_len = 0;
    }
}

pub struct RsaPrivateKeyResult {
    pub ok: bool,
    pub key: RsaPrivateKey,
    pub err: i64,
}

/// Computes the exact RSA modulus bit length (e.g., 1024, 2048, 4096).
pub fn modulusBitLen(pub_key: RsaPublicKey): usize {
    if (pub_key.n_len == 0 || pub_key.n_len > PK1_MAX_MODULUS_LEN) {
        return 0;
    }
    let idx: usize = 0;
    while (idx < pub_key.n_len && pub_key.n[idx] == (0 as u8)) {
        idx = idx + 1;
    }
    if (idx == pub_key.n_len) {
        return 0;
    }
    let top = pub_key.n[idx];
    let top_bits: usize = 0;
    while (top > (0 as u8)) {
        top_bits = top_bits + 1;
        top = top >> (1 as u3);
    }
    return (pub_key.n_len - idx - (1 as usize)) * (8 as usize) + top_bits;
}

/// Encodes an RFC 8017 `RSAPublicKey` into canonical DER bytes via `@cap/der-asn1`.
/// Returns bytes written on success, or a negative `PK1_ERR_*` code.
pub fn encodeRsaPublicKeyDer(pub_key: RsaPublicKey, out: []u8): i64 {
    if (pub_key.n_len == 0 || pub_key.n_len > PK1_MAX_MODULUS_LEN) {
        return PK1_ERR_KEY_TOO_LONG;
    }
    let body_buf: [560]u8 = [0; 560];
    let body_w = DerWriter.new(body_buf[0..560]);
    if (!pk1WriteBigUint(&body_w, pub_key.n[0..pub_key.n_len])) {
        return PK1_ERR_BUFFER_TOO_SMALL;
    }
    if (!body_w.writeU64(pub_key.e)) {
        return PK1_ERR_BUFFER_TOO_SMALL;
    }

    let outer_w = DerWriter.new(out);
    if (!outer_w.writeSequence(body_w.written())) {
        return PK1_ERR_BUFFER_TOO_SMALL;
    }
    return outer_w.len() as i64;
}

/// Decodes an RFC 8017 DER `RSAPublicKey` via `@cap/der-asn1`.
pub fn decodeRsaPublicKeyDer(der: []const u8): RsaPublicKeyResult {
    const empty = RsaPublicKey.empty();
    let outer_r = DerReader.new(der);
    const seq = outer_r.readSequence();
    if (!seq.ok) {
        return RsaPublicKeyResult { ok: false, key: empty, err: PK1_ERR_INVALID_DER };
    }
    if (!outer_r.isEmpty()) {
        return RsaPublicKeyResult { ok: false, key: empty, err: PK1_ERR_TRAILING_DATA };
    }

    let sub = seq.reader;
    let key = RsaPublicKey.empty();
    const n_err = pk1ReadBigUint(&sub, key.n[0..PK1_MAX_MODULUS_LEN], &key.n_len);
    if (n_err != PK1_OK) {
        return RsaPublicKeyResult { ok: false, key: empty, err: n_err };
    }
    const e_res = sub.readU64();
    if (!e_res.ok) {
        return RsaPublicKeyResult { ok: false, key: empty, err: PK1_ERR_INVALID_INTEGER };
    }
    if (!sub.isEmpty()) {
        return RsaPublicKeyResult { ok: false, key: empty, err: PK1_ERR_TRAILING_DATA };
    }
    key.e = e_res.value;
    return RsaPublicKeyResult { ok: true, key: key, err: PK1_OK };
}

/// Encodes an `RSAPublicKey` into RFC 7468 PEM (`-----BEGIN RSA PUBLIC KEY-----`).
pub fn encodeRsaPublicKeyPem(pub_key: RsaPublicKey, crlf: bool, out: []u8): i64 {
    let der_buf: [600]u8 = [0; 600];
    const dlen = encodeRsaPublicKeyDer(pub_key, der_buf[0..600]);
    if (dlen < 0) {
        return dlen;
    }
    return encodePem(PK1_PEM_PUBLIC_LABEL, der_buf[0..(dlen as usize)], crlf, out);
}

/// Decodes an RFC 7468 PEM `RSA PUBLIC KEY` block into `RsaPublicKey`.
/// Strictly verifies that the PEM label is `"RSA PUBLIC KEY"`.
pub fn decodeRsaPublicKeyPem(pem: string): RsaPublicKeyResult {
    const empty = RsaPublicKey.empty();
    let label_buf: [64]u8 = [0; 64];
    let label_len: usize = 0;
    let der_buf: [600]u8 = [0; 600];

    const dlen = decodePem(pem, label_buf[0..64], &label_len, der_buf[0..600]);
    if (dlen < 0) {
        return RsaPublicKeyResult { ok: false, key: empty, err: PK1_ERR_INVALID_PEM };
    }
    if (!pk1StrEqBytes(PK1_PEM_PUBLIC_LABEL, label_buf[0..label_len])) {
        return RsaPublicKeyResult { ok: false, key: empty, err: PK1_ERR_INVALID_PEM_LABEL };
    }
    return decodeRsaPublicKeyDer(der_buf[0..(dlen as usize)]);
}

/// Encodes an RFC 8017 two-prime `RSAPrivateKey` (9 fields) into canonical DER bytes via `@cap/der-asn1`.
/// Returns bytes written on success, or a negative `PK1_ERR_*` code.
pub fn encodeRsaPrivateKeyDer(priv_key: RsaPrivateKey, out: []u8): i64 {
    if (priv_key.version != PK1_VERSION_TWO_PRIME) {
        return PK1_ERR_INVALID_VERSION;
    }
    if (priv_key.n_len == 0 || priv_key.n_len > PK1_MAX_MODULUS_LEN
        || priv_key.d_len == 0 || priv_key.d_len > PK1_MAX_MODULUS_LEN
        || priv_key.p_len == 0 || priv_key.p_len > PK1_MAX_PRIME_LEN
        || priv_key.q_len == 0 || priv_key.q_len > PK1_MAX_PRIME_LEN
        || priv_key.dp_len == 0 || priv_key.dp_len > PK1_MAX_PRIME_LEN
        || priv_key.dq_len == 0 || priv_key.dq_len > PK1_MAX_PRIME_LEN
        || priv_key.qinv_len == 0 || priv_key.qinv_len > PK1_MAX_PRIME_LEN) {
        return PK1_ERR_KEY_TOO_LONG;
    }

    let body_buf: [2560]u8 = [0; 2560];
    let bw = DerWriter.new(body_buf[0..2560]);

    let ok = bw.writeU64(priv_key.version as u64)
        && pk1WriteBigUint(&bw, priv_key.n[0..priv_key.n_len])
        && bw.writeU64(priv_key.e)
        && pk1WriteBigUint(&bw, priv_key.d[0..priv_key.d_len])
        && pk1WriteBigUint(&bw, priv_key.p[0..priv_key.p_len])
        && pk1WriteBigUint(&bw, priv_key.q[0..priv_key.q_len])
        && pk1WriteBigUint(&bw, priv_key.dp[0..priv_key.dp_len])
        && pk1WriteBigUint(&bw, priv_key.dq[0..priv_key.dq_len])
        && pk1WriteBigUint(&bw, priv_key.qinv[0..priv_key.qinv_len]);

    if (!ok) {
        zeroizeBytes(body_buf[0..2560]);
        return PK1_ERR_BUFFER_TOO_SMALL;
    }

    let outer_w = DerWriter.new(out);
    const seq_ok = outer_w.writeSequence(bw.written());
    zeroizeBytes(body_buf[0..2560]);
    if (!seq_ok) {
        return PK1_ERR_BUFFER_TOO_SMALL;
    }
    return outer_w.len() as i64;
}

/// Decodes an RFC 8017 DER two-prime `RSAPrivateKey` (9 fields) via `@cap/der-asn1`.
pub fn decodeRsaPrivateKeyDer(der: []const u8): RsaPrivateKeyResult {
    const empty = RsaPrivateKey.empty();
    let outer_r = DerReader.new(der);
    const seq = outer_r.readSequence();
    if (!seq.ok) {
        return RsaPrivateKeyResult { ok: false, key: empty, err: PK1_ERR_INVALID_DER };
    }
    if (!outer_r.isEmpty()) {
        return RsaPrivateKeyResult { ok: false, key: empty, err: PK1_ERR_TRAILING_DATA };
    }

    let sub = seq.reader;
    const ver_res = sub.readU64();
    if (!ver_res.ok || ver_res.value != (PK1_VERSION_TWO_PRIME as u64)) {
        return RsaPrivateKeyResult { ok: false, key: empty, err: PK1_ERR_INVALID_VERSION };
    }

    let key = RsaPrivateKey.empty();
    key.version = PK1_VERSION_TWO_PRIME;

    const e_n = pk1ReadBigUint(&sub, key.n[0..PK1_MAX_MODULUS_LEN], &key.n_len);
    if (e_n != PK1_OK) {
        key.zeroize();
        return RsaPrivateKeyResult { ok: false, key: empty, err: e_n };
    }
    const e_res = sub.readU64();
    if (!e_res.ok) {
        key.zeroize();
        return RsaPrivateKeyResult { ok: false, key: empty, err: PK1_ERR_INVALID_INTEGER };
    }
    key.e = e_res.value;

    const e_d = pk1ReadBigUint(&sub, key.d[0..PK1_MAX_MODULUS_LEN], &key.d_len);
    if (e_d != PK1_OK) {
        key.zeroize();
        return RsaPrivateKeyResult { ok: false, key: empty, err: e_d };
    }
    const e_p = pk1ReadBigUint(&sub, key.p[0..PK1_MAX_PRIME_LEN], &key.p_len);
    if (e_p != PK1_OK) {
        key.zeroize();
        return RsaPrivateKeyResult { ok: false, key: empty, err: e_p };
    }
    const e_q = pk1ReadBigUint(&sub, key.q[0..PK1_MAX_PRIME_LEN], &key.q_len);
    if (e_q != PK1_OK) {
        key.zeroize();
        return RsaPrivateKeyResult { ok: false, key: empty, err: e_q };
    }
    const e_dp = pk1ReadBigUint(&sub, key.dp[0..PK1_MAX_PRIME_LEN], &key.dp_len);
    if (e_dp != PK1_OK) {
        key.zeroize();
        return RsaPrivateKeyResult { ok: false, key: empty, err: e_dp };
    }
    const e_dq = pk1ReadBigUint(&sub, key.dq[0..PK1_MAX_PRIME_LEN], &key.dq_len);
    if (e_dq != PK1_OK) {
        key.zeroize();
        return RsaPrivateKeyResult { ok: false, key: empty, err: e_dq };
    }
    const e_qi = pk1ReadBigUint(&sub, key.qinv[0..PK1_MAX_PRIME_LEN], &key.qinv_len);
    if (e_qi != PK1_OK) {
        key.zeroize();
        return RsaPrivateKeyResult { ok: false, key: empty, err: e_qi };
    }
    if (!sub.isEmpty()) {
        key.zeroize();
        return RsaPrivateKeyResult { ok: false, key: empty, err: PK1_ERR_TRAILING_DATA };
    }
    return RsaPrivateKeyResult { ok: true, key: key, err: PK1_OK };
}

/// Encodes an `RSAPrivateKey` into RFC 7468 PEM (`-----BEGIN RSA PRIVATE KEY-----`).
pub fn encodeRsaPrivateKeyPem(priv_key: RsaPrivateKey, crlf: bool, out: []u8): i64 {
    let der_buf: [2600]u8 = [0; 2600];
    const dlen = encodeRsaPrivateKeyDer(priv_key, der_buf[0..2600]);
    if (dlen < 0) {
        zeroizeBytes(der_buf[0..2600]);
        return dlen;
    }
    const res = encodePem(PK1_PEM_PRIVATE_LABEL, der_buf[0..(dlen as usize)], crlf, out);
    zeroizeBytes(der_buf[0..2600]);
    return res;
}

/// Decodes an RFC 7468 PEM `RSA PRIVATE KEY` block into `RsaPrivateKey`.
/// Strictly verifies that the PEM label is `"RSA PRIVATE KEY"`.
pub fn decodeRsaPrivateKeyPem(pem: string): RsaPrivateKeyResult {
    const empty = RsaPrivateKey.empty();
    let label_buf: [64]u8 = [0; 64];
    let label_len: usize = 0;
    let der_buf: [2600]u8 = [0; 2600];

    const dlen = decodePem(pem, label_buf[0..64], &label_len, der_buf[0..2600]);
    if (dlen < 0) {
        zeroizeBytes(der_buf[0..2600]);
        return RsaPrivateKeyResult { ok: false, key: empty, err: PK1_ERR_INVALID_PEM };
    }
    if (!pk1StrEqBytes(PK1_PEM_PRIVATE_LABEL, label_buf[0..label_len])) {
        zeroizeBytes(der_buf[0..2600]);
        return RsaPrivateKeyResult { ok: false, key: empty, err: PK1_ERR_INVALID_PEM_LABEL };
    }
    const res = decodeRsaPrivateKeyDer(der_buf[0..(dlen as usize)]);
    zeroizeBytes(der_buf[0..2600]);
    return res;
}
