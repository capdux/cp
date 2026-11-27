// @cap/rfc6979 — Pure CP zero-allocation RFC 6979 deterministic ECDSA nonce generator (HMAC-SHA256 DRBG)
// Parity with RustCrypto `rfc6979` (RFC 6979 Section 3.2 & Section 3.6 extra data).
// Reuses `hmacSha256` from `@cap/cookie-signature`.

import { hmacSha256 } from "../../cookie-signature/src/index.cp";

fn rfcCopy32(dst: []u8, src: []u8) {
    let i: usize = 0;
    while (i < 32) {
        dst[i] = src[i];
        i = i + 1;
    }
}

fn rfcCmpBe32(a: []u8, b: []u8): i32 {
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

fn rfcIsNonZero32(a: []u8): bool {
    let acc: u8 = 0;
    let i: usize = 0;
    while (i < 32) {
        acc = acc | a[i];
        i = i + 1;
    }
    return acc != 0;
}

fn rfcOrderValid32(q: []u8): bool {
    // Must be > 1 so that a valid k with 1 <= k < q exists
    let i: usize = 0;
    while (i < 31) {
        if (q[i] != 0) {
            return true;
        }
        i = i + 1;
    }
    return q[31] > 1;
}

fn rfcSubBe32(a: []u8, b: []u8, out: []u8) {
    let borrow: i32 = 0;
    let i: usize = 32;
    while (i > 0) {
        i = i - 1;
        let diff: i32 = (a[i] as i32) - (b[i] as i32) - borrow;
        if (diff < 0) {
            out[i] = (diff + 256) as u8;
            borrow = 1;
        } else {
            out[i] = diff as u8;
            borrow = 0;
        }
    }
}

pub struct HmacDrbg256 {
    pub k: [32]u8,
    pub v: [32]u8,

    // RFC 6979 Section 3.2 steps b..g (with optional Section 3.6 extra data)
    pub static fn init(priv_key_32: []u8, msg_hash_32: []u8, extra: []u8): HmacDrbg256 {
        let k: [32]u8 = undefined;
        let v: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            k[i] = 0x00;
            v[i] = 0x01;
            i = i + 1;
        }

        // Assemble: V (32) || sep (1) || priv_key_32 || msg_hash_32 || extra
        let msg: [256]u8 = undefined;
        let pos: usize = 0;
        i = 0;
        while (i < 32) {
            msg[pos] = v[i];
            pos = pos + 1;
            i = i + 1;
        }
        msg[pos] = 0x00;
        pos = pos + 1;

        let pk_len = priv_key_32.len;
        if (pk_len > 32) {
            pk_len = 32;
        }
        i = 0;
        while (i < pk_len) {
            msg[pos] = priv_key_32[i];
            pos = pos + 1;
            i = i + 1;
        }

        let mh_len = msg_hash_32.len;
        if (mh_len > 32) {
            mh_len = 32;
        }
        i = 0;
        while (i < mh_len) {
            msg[pos] = msg_hash_32[i];
            pos = pos + 1;
            i = i + 1;
        }

        let ex_len = extra.len;
        if (pos + ex_len > 256) {
            ex_len = (256 as usize) - pos;
        }
        i = 0;
        while (i < ex_len) {
            msg[pos] = extra[i];
            pos = pos + 1;
            i = i + 1;
        }

        let tmp_k: [32]u8 = undefined;
        let tmp_v: [32]u8 = undefined;

        // Step d: K = HMAC(K, V || 0x00 || int2octets(x) || bits2octets(h1) || extra)
        hmacSha256(k[0..32], msg[0..pos], tmp_k[0..32]);
        rfcCopy32(k[0..32], tmp_k[0..32]);

        // Step e: V = HMAC(K, V)
        hmacSha256(k[0..32], v[0..32], tmp_v[0..32]);
        rfcCopy32(v[0..32], tmp_v[0..32]);

        // Step f: K = HMAC(K, V || 0x01 || int2octets(x) || bits2octets(h1) || extra)
        rfcCopy32(msg[0..32], v[0..32]);
        msg[32] = 0x01;
        hmacSha256(k[0..32], msg[0..pos], tmp_k[0..32]);
        rfcCopy32(k[0..32], tmp_k[0..32]);

        // Step g: V = HMAC(K, V)
        hmacSha256(k[0..32], v[0..32], tmp_v[0..32]);
        rfcCopy32(v[0..32], tmp_v[0..32]);

        return HmacDrbg256 { k: k, v: v };
    }

    // RFC 6979 Section 3.2.h: output 32-byte candidate and advance (K, V)
    pub fn nextBytes(*_, out: []u8): usize {
        if (out.len == 0) {
            return 0;
        }
        let tmp_v: [32]u8 = undefined;
        hmacSha256(_.k[0..32], _.v[0..32], tmp_v[0..32]);
        rfcCopy32(_.v[0..32], tmp_v[0..32]);

        let n: usize = 32;
        if (out.len < n) {
            n = out.len;
        }
        let i: usize = 0;
        while (i < n) {
            out[i] = _.v[i];
            i = i + 1;
        }

        // Step h.3: K = HMAC(K, V || 0x00), V = HMAC(K, V)
        let step_buf: [33]u8 = undefined;
        rfcCopy32(step_buf[0..32], _.v[0..32]);
        step_buf[32] = 0x00;

        let tmp_k: [32]u8 = undefined;
        hmacSha256(_.k[0..32], step_buf[0..33], tmp_k[0..32]);
        rfcCopy32(_.k[0..32], tmp_k[0..32]);

        hmacSha256(_.k[0..32], _.v[0..32], tmp_v[0..32]);
        rfcCopy32(_.v[0..32], tmp_v[0..32]);

        return n;
    }
}

pub fn generateK(
    priv_key_32: []u8,
    msg_hash_32: []u8,
    order_32: []u8,
    out_k32: []u8
): bool {
    if (priv_key_32.len != 32 || msg_hash_32.len != 32 || order_32.len != 32 || out_k32.len < 32) {
        return false;
    }
    if (!rfcOrderValid32(order_32)) {
        return false;
    }

    // bits2octets(h1): reduce msg_hash_32 mod order_32 if >= order_32
    let h1: [32]u8 = undefined;
    rfcCopy32(h1[0..32], msg_hash_32);
    if (rfcCmpBe32(h1[0..32], order_32) >= 0) {
        let reduced: [32]u8 = undefined;
        rfcSubBe32(h1[0..32], order_32, reduced[0..32]);
        rfcCopy32(h1[0..32], reduced[0..32]);
    }

    let empty: [1]u8 = [0];
    let drbg = HmacDrbg256.init(priv_key_32, h1[0..32], empty[0..0]);
    let cand: [32]u8 = undefined;

    let attempt: usize = 0;
    while (attempt < 64) {
        let n = drbg.nextBytes(cand[0..32]);
        if (n == 32 && rfcIsNonZero32(cand[0..32]) && rfcCmpBe32(cand[0..32], order_32) < 0) {
            rfcCopy32(out_k32, cand[0..32]);
            return true;
        }
        attempt = attempt + 1;
    }
    return false;
}

export {
    HmacDrbg256,
    generateK
};
