import { HmacSha256, HMAC_SHA256_LEN } from "../../hmac/src/index.cp";

pub const HKDF_HASH_LEN: usize = 32;
pub const HKDF_MAX_OKM_LEN: usize = 255 * 32; // 8160 bytes per RFC 5869

pub struct HkdfSha256 {
    pub prk: [32]u8,

    pub static fn extract(salt: []u8, ikm: []u8): HkdfSha256 {
        let zero_salt: [32]u8 = [0; 32];
        let mac = if (salt.len == 0) {
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
        let i: usize = 0;
        while (i < 32) {
            buf[i] = prk_in[i];
            i = i + 1;
        }
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
            let i: usize = 0;
            while (i < take) {
                okm[out_pos + i] = prev_t[i];
                i = i + 1;
            }
            out_pos = out_pos + take;
            ctr = ctr + 1;
        }
        return true;
    }
}

pub fn hkdfSha256(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool {
    let hk = HkdfSha256.extract(salt, ikm);
    return hk.expand(info, okm);
}
