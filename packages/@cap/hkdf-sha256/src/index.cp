// @cap/hkdf-sha256 — Pure CP zero-allocation RFC 5869 HKDF-SHA256
// Parity with RustCrypto `hkdf` (RFC 5869 Section 2.2 Extract & Section 2.3 Expand).
// Reuses `hmacSha256` & streaming SHA-256 primitives from `@cap/cookie-signature`.

import {
    hmacSha256,
    sha256Init,
    sha256Update,
    sha256Final
} from "../../cookie-signature/src/index.cp";

pub const HKDF_HASH_LEN: usize = 32;
pub const HKDF_MAX_OKM_LEN: usize = 8160; // 255 * 32

fn hkdfStreamingHmac(prk32: []u8, prev_t: []u8, info: []u8, counter: u8, out32: []u8) {
    let ipad: [64]u8 = undefined;
    let opad: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 32) {
        ipad[i] = prk32[i] ^ 0x36;
        opad[i] = prk32[i] ^ 0x5C;
        i = i + 1;
    }
    while (i < 64) {
        ipad[i] = 0x36;
        opad[i] = 0x5C;
        i = i + 1;
    }

    let cbuf: [1]u8 = [counter];
    let inner = sha256Init();
    sha256Update(&inner, ipad[0..64]);
    if (prev_t.len > 0) {
        sha256Update(&inner, prev_t);
    }
    if (info.len > 0) {
        sha256Update(&inner, info);
    }
    sha256Update(&inner, cbuf[0..1]);
    let inner_hash: [32]u8 = undefined;
    sha256Final(&inner, inner_hash[0..32]);

    let outer = sha256Init();
    sha256Update(&outer, opad[0..64]);
    sha256Update(&outer, inner_hash[0..32]);
    sha256Final(&outer, out32);
}

// RFC 5869 Section 2.2: HKDF-Extract(salt, IKM) -> PRK (32 bytes)
pub fn hkdfExtract(salt: []u8, ikm: []u8, out_prk32: []u8): bool {
    if (out_prk32.len < HKDF_HASH_LEN) {
        return false;
    }
    if (salt.len == 0) {
        let zero_salt: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            zero_salt[i] = 0;
            i = i + 1;
        }
        hmacSha256(zero_salt[0..32], ikm, out_prk32[0..32]);
        return true;
    }
    hmacSha256(salt, ikm, out_prk32[0..32]);
    return true;
}

// RFC 5869 Section 2.3: HKDF-Expand(PRK, info, L) -> OKM (0 <= L <= 8160)
pub fn hkdfExpand(prk32: []u8, info: []u8, out_okm: []u8): bool {
    if (prk32.len < HKDF_HASH_LEN) {
        return false;
    }
    if (out_okm.len > HKDF_MAX_OKM_LEN) {
        return false;
    }
    if (out_okm.len == 0) {
        return true;
    }

    let n_blocks: usize = (out_okm.len + HKDF_HASH_LEN - (1 as usize)) / HKDF_HASH_LEN;
    let t_prev: [32]u8 = undefined;
    let t_cur: [32]u8 = undefined;
    let msg_buf: [512]u8 = undefined;

    let out_pos: usize = 0;
    let blk: usize = 1;
    while (blk <= n_blocks) {
        let prev_len: usize = 0;
        if (blk > 1) {
            prev_len = 32;
        }
        let counter: u8 = blk as u8;

        if (prev_len + info.len + (1 as usize) <= 512) {
            let mpos: usize = 0;
            let j: usize = 0;
            while (j < prev_len) {
                msg_buf[mpos] = t_prev[j];
                mpos = mpos + 1;
                j = j + 1;
            }
            j = 0;
            while (j < info.len) {
                msg_buf[mpos] = info[j];
                mpos = mpos + 1;
                j = j + 1;
            }
            msg_buf[mpos] = counter;
            mpos = mpos + 1;
            hmacSha256(prk32[0..32], msg_buf[0..mpos], t_cur[0..32]);
        } else {
            hkdfStreamingHmac(prk32[0..32], t_prev[0..prev_len], info, counter, t_cur[0..32]);
        }

        let rem = out_okm.len - out_pos;
        let copy_len: usize = 32;
        if (rem < copy_len) {
            copy_len = rem;
        }
        let k: usize = 0;
        while (k < copy_len) {
            out_okm[out_pos + k] = t_cur[k];
            k = k + 1;
        }
        out_pos = out_pos + copy_len;

        k = 0;
        while (k < 32) {
            t_prev[k] = t_cur[k];
            k = k + 1;
        }
        blk = blk + 1;
    }

    return true;
}

// One-shot HKDF-Extract + HKDF-Expand
pub fn hkdfDerive(salt: []u8, ikm: []u8, info: []u8, out_okm: []u8): bool {
    if (out_okm.len > HKDF_MAX_OKM_LEN) {
        return false;
    }
    let prk: [32]u8 = undefined;
    if (!hkdfExtract(salt, ikm, prk[0..32])) {
        return false;
    }
    return hkdfExpand(prk[0..32], info, out_okm);
}

export {
    HKDF_HASH_LEN,
    HKDF_MAX_OKM_LEN,
    hkdfExtract,
    hkdfExpand,
    hkdfDerive
};
