// @cap/cookie-signature — Zero-allocation HMAC-SHA256 cookie signing & verification
// Reuses @cap/base64ct for constant-time unpadded Base64 encoding (MIT License)

import { encodeStdUnpadded } from "../../base64ct/src/index.cp";

const SHA256_K: [64]u32 = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];

fn add32(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & 0xFFFFFFFF) as u32;
}

fn rotr32(x: u32, n: u32): u32 {
    let left_shift: u32 = (32 as u32) - n;
    return (x >> n) | (x << left_shift);
}

struct Sha256State {
    h: [8]u32,
    buf: [64]u8,
    buf_len: usize,
    total_bytes: u64,
}

fn sha256Init(): Sha256State {
    return Sha256State {
        h: [
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
        ],
        buf: [0; 64],
        buf_len: 0,
        total_bytes: 0,
    };
}

fn sha256Compress(st: *Sha256State, block: []const u8) {
    let w: [64]u32 = [0; 64];
    let i: usize = 0;
    while (i < 16) {
        let base: usize = i * 4;
        w[i] = ((block[base] as u32) << 24) |
            ((block[base + 1] as u32) << 16) |
            ((block[base + 2] as u32) << 8) |
            (block[base + 3] as u32);
        i = i + 1;
    }
    let t: usize = 16;
    while (t < 64) {
        let w15: u32 = w[t - 15];
        let s0: u32 = rotr32(w15, 7) ^ rotr32(w15, 18) ^ (w15 >> 3);
        let w2: u32 = w[t - 2];
        let s1: u32 = rotr32(w2, 17) ^ rotr32(w2, 19) ^ (w2 >> 10);
        w[t] = add32(add32(w[t - 16], s0), add32(w[t - 7], s1));
        t = t + 1;
    }

    let a: u32 = st.*.h[0];
    let b: u32 = st.*.h[1];
    let c: u32 = st.*.h[2];
    let d: u32 = st.*.h[3];
    let e: u32 = st.*.h[4];
    let f: u32 = st.*.h[5];
    let g: u32 = st.*.h[6];
    let hh: u32 = st.*.h[7];

    let r: usize = 0;
    while (r < 64) {
        let big_s1: u32 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        let ch: u32 = (e & f) ^ ((~e) & g);
        let temp1: u32 = add32(add32(add32(hh, big_s1), ch), add32(SHA256_K[r], w[r]));
        let big_s0: u32 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        let maj: u32 = (a & b) ^ (a & c) ^ (b & c);
        let temp2: u32 = add32(big_s0, maj);

        hh = g;
        g = f;
        f = e;
        e = add32(d, temp1);
        d = c;
        c = b;
        b = a;
        a = add32(temp1, temp2);
        r = r + 1;
    }

    st.*.h[0] = add32(st.*.h[0], a);
    st.*.h[1] = add32(st.*.h[1], b);
    st.*.h[2] = add32(st.*.h[2], c);
    st.*.h[3] = add32(st.*.h[3], d);
    st.*.h[4] = add32(st.*.h[4], e);
    st.*.h[5] = add32(st.*.h[5], f);
    st.*.h[6] = add32(st.*.h[6], g);
    st.*.h[7] = add32(st.*.h[7], hh);
}

fn sha256Update(st: *Sha256State, data: []const u8) {
    st.*.total_bytes = st.*.total_bytes + (data.len as u64);
    let i: usize = 0;
    while (i < data.len) {
        st.*.buf[st.*.buf_len] = data[i];
        st.*.buf_len = st.*.buf_len + 1;
        if (st.*.buf_len == 64) {
            let blk: [64]u8 = st.*.buf;
            sha256Compress(st, blk[0..64]);
            st.*.buf_len = 0;
        }
        i = i + 1;
    }
}

fn sha256Final(st: *Sha256State, out32: []u8) {
    let bit_len: u64 = st.*.total_bytes * 8;
    let pad_one: [1]u8 = [0x80];
    st.*.buf[st.*.buf_len] = pad_one[0];
    st.*.buf_len = st.*.buf_len + 1;

    if (st.*.buf_len > 56) {
        while (st.*.buf_len < 64) {
            st.*.buf[st.*.buf_len] = 0;
            st.*.buf_len = st.*.buf_len + 1;
        }
        let blk1: [64]u8 = st.*.buf;
        sha256Compress(st, blk1[0..64]);
        st.*.buf_len = 0;
    }

    while (st.*.buf_len < 56) {
        st.*.buf[st.*.buf_len] = 0;
        st.*.buf_len = st.*.buf_len + 1;
    }

    st.*.buf[56] = ((bit_len >> 56) & 0xFF) as u8;
    st.*.buf[57] = ((bit_len >> 48) & 0xFF) as u8;
    st.*.buf[58] = ((bit_len >> 40) & 0xFF) as u8;
    st.*.buf[59] = ((bit_len >> 32) & 0xFF) as u8;
    st.*.buf[60] = ((bit_len >> 24) & 0xFF) as u8;
    st.*.buf[61] = ((bit_len >> 16) & 0xFF) as u8;
    st.*.buf[62] = ((bit_len >> 8) & 0xFF) as u8;
    st.*.buf[63] = (bit_len & 0xFF) as u8;
    let blk2: [64]u8 = st.*.buf;
    sha256Compress(st, blk2[0..64]);

    if (out32.len < 32) {
        return;
    }
    let i: usize = 0;
    while (i < 8) {
        let word: u32 = st.*.h[i];
        let base: usize = i * 4;
        out32[base] = ((word >> 24) & 0xFF) as u8;
        out32[base + 1] = ((word >> 16) & 0xFF) as u8;
        out32[base + 2] = ((word >> 8) & 0xFF) as u8;
        out32[base + 3] = (word & 0xFF) as u8;
        i = i + 1;
    }
}

pub fn sha256(data: []const u8, out32: []u8) {
    let st: Sha256State = sha256Init();
    sha256Update(&st, data);
    sha256Final(&st, out32);
}

pub fn hmacSha256(key: string, msg: string, out32: []u8) {
    let key_block: [64]u8 = [0; 64];
    if (key.len > 64) {
        sha256(key, key_block[0..32]);
    } else {
        let i: usize = 0;
        while (i < key.len) {
            key_block[i] = key[i];
            i = i + 1;
        }
    }

    let ipad: [64]u8 = [0; 64];
    let opad: [64]u8 = [0; 64];
    let k: usize = 0;
    while (k < 64) {
        ipad[k] = key_block[k] ^ 0x36;
        opad[k] = key_block[k] ^ 0x5c;
        k = k + 1;
    }

    let inner_hash: [32]u8 = [0; 32];
    let st_in: Sha256State = sha256Init();
    sha256Update(&st_in, ipad[0..64]);
    sha256Update(&st_in, msg);
    sha256Final(&st_in, inner_hash[0..32]);

    let st_out: Sha256State = sha256Init();
    sha256Update(&st_out, opad[0..64]);
    sha256Update(&st_out, inner_hash[0..32]);
    sha256Final(&st_out, out32);
}

pub fn ctEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff = diff | (a[i] ^ b[i]);
        i = i + 1;
    }
    return diff == 0;
}

pub struct UnsignResult {
    pub ok: bool,
    pub val_len: usize,
}

pub fn sign(val: string, secret: string, out: []u8): usize {
    let mac: [32]u8 = [0; 32];
    hmacSha256(secret, val, mac[0..32]);

    let sig_b64: [44]u8 = [0; 44];
    let sig_len: usize = encodeStdUnpadded(mac[0..32], sig_b64[0..44]);
    if (sig_len == 0) {
        return 0;
    }

    let total: usize = val.len + 1 + sig_len;
    if (out.len < total) {
        return 0;
    }

    let i: usize = 0;
    while (i < val.len) {
        out[i] = val[i];
        i = i + 1;
    }
    out[val.len] = 46; // '.'
    let j: usize = 0;
    while (j < sig_len) {
        out[val.len + 1 + j] = sig_b64[j];
        j = j + 1;
    }
    return total;
}

pub fn unsign(signed_cookie: string, secret: string): UnsignResult {
    // Find last '.' delimiter
    let dot_idx: i64 = -1;
    let i: usize = signed_cookie.len;
    while (i > 0) {
        i = i - 1;
        if (signed_cookie[i] == 46) {
            dot_idx = i as i64;
            break;
        }
    }
    if (dot_idx < 0) {
        return UnsignResult { ok: false, val_len: 0 };
    }

    let val_len: usize = dot_idx as usize;
    let val_slice: string = signed_cookie[0..val_len];
    let given_sig: string = signed_cookie[(val_len + 1)..signed_cookie.len];

    let mac: [32]u8 = [0; 32];
    hmacSha256(secret, val_slice, mac[0..32]);

    let expected_sig: [44]u8 = [0; 44];
    let expected_len: usize = encodeStdUnpadded(mac[0..32], expected_sig[0..44]);
    if (expected_len == 0) {
        return UnsignResult { ok: false, val_len: 0 };
    }

    if (ctEqual(given_sig, expected_sig[0..expected_len])) {
        return UnsignResult { ok: true, val_len: val_len };
    }
    return UnsignResult { ok: false, val_len: 0 };
}
