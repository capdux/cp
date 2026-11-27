// @cap/hkdf — Zero-heap RFC 5869 HKDF (Extract / Expand / Derive) supporting SHA-256, SHA-384 & SHA-512.
// Clean-room MIT implementation in pure CP, reusing `cap:crypto`, `cap:bin`, and `@cap/hmac`.

import { HmacSha256, HMAC_SHA256_LEN } from "../../hmac/src/index.cp";
import { ctEqual } from "../../cookie-signature/src/index.cp";

pub const HKDF_HASH_LEN: usize = 32;
pub const HKDF_MAX_OKM_LEN: usize = 255 * 32; // 8160 bytes per RFC 5869 (SHA-256)

pub const HKDF_SHA256_HASH_LEN: usize = 32;
pub const HKDF_SHA256_MAX_OKM_LEN: usize = 255 * 32; // 8160 bytes
pub const HKDF_SHA384_HASH_LEN: usize = 48;
pub const HKDF_SHA384_MAX_OKM_LEN: usize = 255 * 48; // 12240 bytes
pub const HKDF_SHA512_HASH_LEN: usize = 64;
pub const HKDF_SHA512_MAX_OKM_LEN: usize = 255 * 64; // 16320 bytes

/// Supported RFC 5869 underlying hash algorithms.
pub enum HkdfHashAlg {
    Sha256,
    Sha384,
    Sha512,

    pub fn hashLen(): usize {
        if (_ == HkdfHashAlg.Sha256) { return HKDF_SHA256_HASH_LEN; }
        if (_ == HkdfHashAlg.Sha384) { return HKDF_SHA384_HASH_LEN; }
        return HKDF_SHA512_HASH_LEN;
    }

    pub fn maxOkmLen(): usize {
        return 255 * _.hashLen();
    }
}

pub struct HkdfSha256 {
    pub prk: [32]u8,

    pub static fn extract(salt: []u8, ikm: []u8): HkdfSha256 {
        let mac = if (salt.len == 0) {
            let zero_salt: [32]u8 = [0; 32];
            HmacSha256.newFromSlice(zero_salt[0..32])
        } else {
            HmacSha256.newFromSlice(salt)
        };
        mac.update(ikm);
        let prk_out: [32]u8 = undefined;
        mac.finalize(prk_out[0..32]);
        return HkdfSha256 { prk: prk_out };
    }

    pub static fn fromPrk(prk_in: []u8): HkdfSha256? {
        if (prk_in.len < HKDF_HASH_LEN) { return null; }
        let buf: [32]u8 = undefined;
        @memcpy(buf[0..32], prk_in[0..32]);
        return HkdfSha256 { prk: buf };
    }

    pub fn expand(info: []u8, okm: []u8): bool {
        if (okm.len > HKDF_MAX_OKM_LEN) { return false; }
        if (okm.len == 0) { return true; }
        let n_blocks = (okm.len + HMAC_SHA256_LEN - 1) / HMAC_SHA256_LEN;
        let prev_t: [32]u8 = undefined;
        let out_pos: usize = 0;
        let ctr: usize = 1;
        let prk_local: [32]u8 = _.prk;
        while (ctr <= n_blocks) {
            let mac = HmacSha256.newFromSlice(prk_local[0..32]);
            if (ctr > 1) {
                mac.update(prev_t[0..32]);
            }
            if (info.len > 0) {
                mac.update(info);
            }
            let c_byte: [1]u8 = [ctr as u8];
            mac.update(c_byte[0..1]);
            mac.finalize(prev_t[0..32]);

            let rem = okm.len - out_pos;
            let take = if (rem > 32) { 32 } else { rem };
            if (take > 0) {
                @memcpy(okm[out_pos..out_pos + take], prev_t[0..take]);
            }
            out_pos = out_pos + take;
            ctr = ctr + 1;
        }
        return true;
    }

    /// RFC 5869 Expand accepting multiple info slices concatenated without intermediate allocation.
    pub fn expandMultiInfo(infos: []const string, okm: []u8): bool {
        if (okm.len > HKDF_MAX_OKM_LEN) { return false; }
        if (okm.len == 0) { return true; }
        let n_blocks = (okm.len + HMAC_SHA256_LEN - 1) / HMAC_SHA256_LEN;
        let prev_t: [32]u8 = undefined;
        let out_pos: usize = 0;
        let ctr: usize = 1;
        let prk_local: [32]u8 = _.prk;
        while (ctr <= n_blocks) {
            let mac = HmacSha256.newFromSlice(prk_local[0..32]);
            if (ctr > 1) {
                mac.update(prev_t[0..32]);
            }
            let j: usize = 0;
            while (j < infos.len) {
                if (infos[j].len > 0) {
                    mac.updateStr(infos[j]);
                }
                j = j + 1;
            }
            let c_byte: [1]u8 = [ctr as u8];
            mac.update(c_byte[0..1]);
            mac.finalize(prev_t[0..32]);

            let rem = okm.len - out_pos;
            let take = if (rem > 32) { 32 } else { rem };
            if (take > 0) {
                @memcpy(okm[out_pos..out_pos + take], prev_t[0..take]);
            }
            out_pos = out_pos + take;
            ctr = ctr + 1;
        }
        return true;
    }

    /// TLS 1.3 HKDF-Expand-Label (RFC 8446 §7.1): HkdfLabel = uint16 length || "tls13 " + label || context.
    pub fn expandLabel(label: []const u8, context: []const u8, okm: []u8): bool {
        if (okm.len > HKDF_MAX_OKM_LEN || okm.len > 65535) { return false; }
        if (6 + label.len > 255 || context.len > 255) { return false; }
        let info_buf: [520]u8 = undefined;
        info_buf[0] = ((okm.len >> 8) & 0xff) as u8;
        info_buf[1] = (okm.len & 0xff) as u8;
        info_buf[2] = (6 + label.len) as u8;
        @memcpy(info_buf[3..9], "tls13 ");
        if (label.len > 0) {
            @memcpy(info_buf[9..9 + label.len], label);
        }
        let ctx_pos = 9 + label.len;
        info_buf[ctx_pos] = context.len as u8;
        if (context.len > 0) {
            @memcpy(info_buf[ctx_pos + 1..ctx_pos + 1 + context.len], context);
        }
        let total = ctx_pos + 1 + context.len;
        return _.expand(info_buf[0..total], okm);
    }

    /// Constant-time comparison of internal PRK against `expected`.
    pub fn verifyPrk(expected: []const u8): bool {
        if (expected.len != HKDF_SHA256_HASH_LEN) { return false; }
        return ctEqual(_.prk[0..32], expected);
    }
}

/// RFC 5869 HKDF-SHA384 (48-byte PRK, up to 12240-byte OKM).
pub struct HkdfSha384 {
    pub prk: [48]u8,

    pub static fn extract(salt: []const u8, ikm: []const u8): HkdfSha384 {
        let prk_out: [48]u8 = undefined;
        if (salt.len == 0) {
            let zero_salt: [48]u8 = [0; 48];
            std.crypto.auth.hmac.sha2.HmacSha384.create(&prk_out, ikm, zero_salt[0..48]);
        } else {
            std.crypto.auth.hmac.sha2.HmacSha384.create(&prk_out, ikm, salt);
        }
        return HkdfSha384 { prk: prk_out };
    }

    pub static fn fromPrk(prk_in: []const u8): HkdfSha384? {
        if (prk_in.len < HKDF_SHA384_HASH_LEN) { return null; }
        let buf: [48]u8 = undefined;
        @memcpy(buf[0..48], prk_in[0..48]);
        return HkdfSha384 { prk: buf };
    }

    pub fn expand(info: []const u8, okm: []u8): bool {
        if (okm.len > HKDF_SHA384_MAX_OKM_LEN) { return false; }
        if (okm.len == 0) { return true; }
        let n_blocks = (okm.len + HKDF_SHA384_HASH_LEN - 1) / HKDF_SHA384_HASH_LEN;
        let prev_t: [48]u8 = undefined;
        let msg_buf: [1073]u8 = undefined; // 48 (prev_t) + up to 1024 (info) + 1 (ctr)
        if (info.len > 1024) { return false; }
        let out_pos: usize = 0;
        let ctr: usize = 1;
        while (ctr <= n_blocks) {
            let mlen: usize = 0;
            if (ctr > 1) {
                @memcpy(msg_buf[0..48], prev_t[0..48]);
                mlen = 48;
            }
            if (info.len > 0) {
                @memcpy(msg_buf[mlen..mlen + info.len], info);
                mlen = mlen + info.len;
            }
            msg_buf[mlen] = ctr as u8;
            mlen = mlen + 1;

            std.crypto.auth.hmac.sha2.HmacSha384.create(&prev_t, msg_buf[0..mlen], _.prk[0..48]);

            let rem = okm.len - out_pos;
            let take = if (rem > 48) { 48 } else { rem };
            if (take > 0) {
                @memcpy(okm[out_pos..out_pos + take], prev_t[0..take]);
            }
            out_pos = out_pos + take;
            ctr = ctr + 1;
        }
        return true;
    }

    pub fn verifyPrk(expected: []const u8): bool {
        if (expected.len != HKDF_SHA384_HASH_LEN) { return false; }
        return ctEqual(_.prk[0..48], expected);
    }
}

/// RFC 5869 HKDF-SHA512 (64-byte PRK, up to 16320-byte OKM).
pub struct HkdfSha512 {
    pub prk: [64]u8,

    pub static fn extract(salt: []const u8, ikm: []const u8): HkdfSha512 {
        let prk_out = std.crypto.kdf.hkdf.HkdfSha512.extract(salt, ikm);
        return HkdfSha512 { prk: prk_out };
    }

    pub static fn fromPrk(prk_in: []const u8): HkdfSha512? {
        if (prk_in.len < HKDF_SHA512_HASH_LEN) { return null; }
        let buf: [64]u8 = undefined;
        @memcpy(buf[0..64], prk_in[0..64]);
        return HkdfSha512 { prk: buf };
    }

    pub fn expand(info: []const u8, okm: []u8): bool {
        if (okm.len > HKDF_SHA512_MAX_OKM_LEN) { return false; }
        if (okm.len == 0) { return true; }
        std.crypto.kdf.hkdf.HkdfSha512.expand(okm, info, _.prk);
        return true;
    }

    pub fn verifyPrk(expected: []const u8): bool {
        if (expected.len != HKDF_SHA512_HASH_LEN) { return false; }
        return ctEqual(_.prk[0..64], expected);
    }
}

pub fn hkdfSha256(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool {
    let hk = HkdfSha256.extract(salt, ikm);
    return hk.expand(info, okm);
}

pub fn hkdfSha384(salt: []const u8, ikm: []const u8, info: []const u8, okm: []u8): bool {
    let hk = HkdfSha384.extract(salt, ikm);
    return hk.expand(info, okm);
}

pub fn hkdfSha512(salt: []const u8, ikm: []const u8, info: []const u8, okm: []u8): bool {
    let hk = HkdfSha512.extract(salt, ikm);
    return hk.expand(info, okm);
}

/// Unified RFC 5869 HKDF-Extract for any supported `HkdfHashAlg`.
/// Writes PRK into `prk_out` and returns the hash length (32/48/64), or 0 if `prk_out` is too small.
pub fn hkdfExtract(alg: HkdfHashAlg, salt: []const u8, ikm: []const u8, prk_out: []u8): usize {
    let hlen = alg.hashLen();
    if (prk_out.len < hlen) { return 0; }
    if (alg == HkdfHashAlg.Sha256) {
        let s_buf: [256]u8 = undefined;
        let i_buf: [256]u8 = undefined;
        if (salt.len <= 256 && ikm.len <= 256) {
            if (salt.len > 0) { @memcpy(s_buf[0..salt.len], salt); }
            if (ikm.len > 0) { @memcpy(i_buf[0..ikm.len], ikm); }
            let hk = HkdfSha256.extract(s_buf[0..salt.len], i_buf[0..ikm.len]);
            @memcpy(prk_out[0..32], hk.prk[0..32]);
            return 32;
        }
        // Fallback for large inputs via HmacSha256 updateStr
        let mac = if (salt.len == 0) {
            let zero_salt: [32]u8 = [0; 32];
            HmacSha256.newFromSlice(zero_salt[0..32])
        } else {
            HmacSha256.newFromKey(salt)
        };
        mac.updateStr(ikm);
        mac.finalize(prk_out[0..32]);
        return 32;
    }
    if (alg == HkdfHashAlg.Sha384) {
        let hk = HkdfSha384.extract(salt, ikm);
        @memcpy(prk_out[0..48], hk.prk[0..48]);
        return 48;
    }
    let hk = HkdfSha512.extract(salt, ikm);
    @memcpy(prk_out[0..64], hk.prk[0..64]);
    return 64;
}

/// Unified RFC 5869 HKDF-Expand for any supported `HkdfHashAlg`.
pub fn hkdfExpand(alg: HkdfHashAlg, prk: []const u8, info: []const u8, okm: []u8): bool {
    let hlen = alg.hashLen();
    if (prk.len < hlen || okm.len > alg.maxOkmLen()) { return false; }
    if (okm.len == 0) { return true; }
    if (alg == HkdfHashAlg.Sha256) {
        let prk_arr: [32]u8 = undefined;
        @memcpy(prk_arr[0..32], prk[0..32]);
        let hk = HkdfSha256 { prk: prk_arr };
        let infos: [1]string = [info];
        return hk.expandMultiInfo(infos[0..1], okm);
    }
    if (alg == HkdfHashAlg.Sha384) {
        let hk_opt = HkdfSha384.fromPrk(prk);
        if (hk_opt == null) { return false; }
        let hk = hk_opt ?? HkdfSha384 { prk: [0; 48] };
        return hk.expand(info, okm);
    }
    let hk_opt = HkdfSha512.fromPrk(prk);
    if (hk_opt == null) { return false; }
    let hk = hk_opt ?? HkdfSha512 { prk: [0; 64] };
    return hk.expand(info, okm);
}

/// Unified RFC 5869 HKDF-Derive (Extract + Expand) for any supported `HkdfHashAlg`.
pub fn hkdfDerive(alg: HkdfHashAlg, salt: []const u8, ikm: []const u8, info: []const u8, okm: []u8): bool {
    if (okm.len > alg.maxOkmLen()) { return false; }
    let prk_buf: [64]u8 = undefined;
    let hlen = hkdfExtract(alg, salt, ikm, prk_buf[0..64]);
    if (hlen == 0) { return false; }
    return hkdfExpand(alg, prk_buf[0..hlen], info, okm);
}
