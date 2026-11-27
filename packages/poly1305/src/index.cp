// @cap/poly1305 — Pure CP zero-allocation RFC 8439 Poly1305 one-time authenticator
// Parity with RustCrypto `poly1305` (Donna 26-bit limbs mod 2^130 - 5, overflow-free in Debug mode).
// Reuses constant-time `ctEqBytes` from `@cap/subtle-ct`.

import { ctEqBytes } from "../../subtle/src/index.cp";

pub const POLY1305_KEY_LEN: usize = 32;
pub const POLY1305_TAG_LEN: usize = 16;

fn p1305ReadU32Le(b: []u8, off: usize): u32 {
    return (b[off] as u32)
        | ((b[off + 1] as u32) << 8)
        | ((b[off + 2] as u32) << 16)
        | ((b[off + 3] as u32) << 24);
}

fn p1305WriteU32Le(out: []u8, off: usize, v: u32) {
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
    out[off + 2] = ((v >> 16) & 0xFF) as u8;
    out[off + 3] = ((v >> 24) & 0xFF) as u8;
}

pub struct Poly1305 {
    pub r0: u32,
    pub r1: u32,
    pub r2: u32,
    pub r3: u32,
    pub r4: u32,
    pub pad0: u32,
    pub pad1: u32,
    pub pad2: u32,
    pub pad3: u32,
    pub h0: u32,
    pub h1: u32,
    pub h2: u32,
    pub h3: u32,
    pub h4: u32,
    pub buf: [16]u8,
    pub buf_len: usize,

    pub static fn init(key32: []u8): Poly1305 {
        let zbuf: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        if (key32.len < 32) {
            return Poly1305 {
                r0: 0, r1: 0, r2: 0, r3: 0, r4: 0,
                pad0: 0, pad1: 0, pad2: 0, pad3: 0,
                h0: 0, h1: 0, h2: 0, h3: 0, h4: 0,
                buf: zbuf,
                buf_len: 0,
            };
        }

        // Clamp r per RFC 8439 Section 2.5 into 5x 26-bit limbs
        let r0: u32 = p1305ReadU32Le(key32, 0) & 0x3ffffff;
        let r1: u32 = (p1305ReadU32Le(key32, 3) >> 2) & 0x3ffff03;
        let r2: u32 = (p1305ReadU32Le(key32, 6) >> 4) & 0x3ffc0ff;
        let r3: u32 = (p1305ReadU32Le(key32, 9) >> 6) & 0x3f03fff;
        let r4: u32 = (p1305ReadU32Le(key32, 12) >> 8) & 0x00fffff;

        let pad0 = p1305ReadU32Le(key32, 16);
        let pad1 = p1305ReadU32Le(key32, 20);
        let pad2 = p1305ReadU32Le(key32, 24);
        let pad3 = p1305ReadU32Le(key32, 28);

        return Poly1305 {
            r0: r0, r1: r1, r2: r2, r3: r3, r4: r4,
            pad0: pad0, pad1: pad1, pad2: pad2, pad3: pad3,
            h0: 0, h1: 0, h2: 0, h3: 0, h4: 0,
            buf: zbuf,
            buf_len: 0,
        };
    }

    fn processBlock(*_, blk: []u8, hibit: u32) {
        let r0: u64 = _.r0 as u64;
        let r1: u64 = _.r1 as u64;
        let r2: u64 = _.r2 as u64;
        let r3: u64 = _.r3 as u64;
        let r4: u64 = _.r4 as u64;

        let s1: u64 = r1 * 5;
        let s2: u64 = r2 * 5;
        let s3: u64 = r3 * 5;
        let s4: u64 = r4 * 5;

        let h0: u64 = (_.h0 as u64) + ((p1305ReadU32Le(blk, 0) & 0x3ffffff) as u64);
        let h1: u64 = (_.h1 as u64) + (((p1305ReadU32Le(blk, 3) >> 2) & 0x3ffffff) as u64);
        let h2: u64 = (_.h2 as u64) + (((p1305ReadU32Le(blk, 6) >> 4) & 0x3ffffff) as u64);
        let h3: u64 = (_.h3 as u64) + (((p1305ReadU32Le(blk, 9) >> 6) & 0x3ffffff) as u64);
        let h4: u64 = (_.h4 as u64) + (((p1305ReadU32Le(blk, 12) >> 8) | hibit) as u64);

        let d0: u64 = h0 * r0 + h1 * s4 + h2 * s3 + h3 * s2 + h4 * s1;
        let d1: u64 = h0 * r1 + h1 * r0 + h2 * s4 + h3 * s3 + h4 * s2;
        let d2: u64 = h0 * r2 + h1 * r1 + h2 * r0 + h3 * s4 + h4 * s3;
        let d3: u64 = h0 * r3 + h1 * r2 + h2 * r1 + h3 * r0 + h4 * s4;
        let d4: u64 = h0 * r4 + h1 * r3 + h2 * r2 + h3 * r1 + h4 * r0;

        let c: u64 = d0 >> 26;
        let nh0: u64 = d0 & 0x3ffffff;

        d1 = d1 + c;
        c = d1 >> 26;
        let nh1: u64 = d1 & 0x3ffffff;

        d2 = d2 + c;
        c = d2 >> 26;
        let nh2: u64 = d2 & 0x3ffffff;

        d3 = d3 + c;
        c = d3 >> 26;
        let nh3: u64 = d3 & 0x3ffffff;

        d4 = d4 + c;
        c = d4 >> 26;
        let nh4: u64 = d4 & 0x3ffffff;

        nh0 = nh0 + c * 5;
        c = nh0 >> 26;
        nh0 = nh0 & 0x3ffffff;
        nh1 = nh1 + c;

        _.h0 = nh0 as u32;
        _.h1 = nh1 as u32;
        _.h2 = nh2 as u32;
        _.h3 = nh3 as u32;
        _.h4 = nh4 as u32;
    }

    pub fn update(*_, data: []u8) {
        let pos: usize = 0;

        if (_.buf_len > 0) {
            let need = (16 as usize) - _.buf_len;
            let take = data.len;
            if (take > need) {
                take = need;
            }
            let i: usize = 0;
            while (i < take) {
                _.buf[_.buf_len + i] = data[i];
                i = i + 1;
            }
            _.buf_len = _.buf_len + take;
            pos = pos + take;
            if (_.buf_len == 16) {
                let full_blk: [16]u8 = undefined;
                i = 0;
                while (i < 16) {
                    full_blk[i] = _.buf[i];
                    i = i + 1;
                }
                _.processBlock(full_blk[0..16], 1 << 24);
                _.buf_len = 0;
            }
        }

        while (pos + 16 <= data.len) {
            _.processBlock(data[pos..(pos + 16)], 1 << 24);
            pos = pos + 16;
        }

        while (pos < data.len) {
            _.buf[_.buf_len] = data[pos];
            _.buf_len = _.buf_len + 1;
            pos = pos + 1;
        }
    }

    pub fn finalize(*_, out_tag16: []u8) {
        if (out_tag16.len < 16) {
            return;
        }

        if (_.buf_len > 0) {
            let last_blk: [16]u8 = undefined;
            let i: usize = 0;
            while (i < _.buf_len) {
                last_blk[i] = _.buf[i];
                i = i + 1;
            }
            last_blk[_.buf_len] = 1;
            i = _.buf_len + 1;
            while (i < 16) {
                last_blk[i] = 0;
                i = i + 1;
            }
            _.processBlock(last_blk[0..16], 0);
            _.buf_len = 0;
        }

        // Full carry chain
        let h0 = _.h0;
        let h1 = _.h1;
        let h2 = _.h2;
        let h3 = _.h3;
        let h4 = _.h4;

        let c: u32 = h1 >> 26;
        h1 = h1 & 0x3ffffff;
        h2 = h2 + c;
        c = h2 >> 26;
        h2 = h2 & 0x3ffffff;
        h3 = h3 + c;
        c = h3 >> 26;
        h3 = h3 & 0x3ffffff;
        h4 = h4 + c;
        c = h4 >> 26;
        h4 = h4 & 0x3ffffff;
        h0 = h0 + c * 5;
        c = h0 >> 26;
        h0 = h0 & 0x3ffffff;
        h1 = h1 + c;

        // Constant-time conditional reduction mod (2^130 - 5)
        let g0: u32 = h0 + 5;
        c = g0 >> 26;
        g0 = g0 & 0x3ffffff;
        let g1: u32 = h1 + c;
        c = g1 >> 26;
        g1 = g1 & 0x3ffffff;
        let g2: u32 = h2 + c;
        c = g2 >> 26;
        g2 = g2 & 0x3ffffff;
        let g3: u32 = h3 + c;
        c = g3 >> 26;
        g3 = g3 & 0x3ffffff;
        let g4_sum: u32 = h4 + c;
        let ge_p: u32 = g4_sum >> 26;
        let g4: u32 = g4_sum & 0x3ffffff;
        let keep_h: u32 = (1 as u32) - ge_p;

        h0 = h0 * keep_h + g0 * ge_p;
        h1 = h1 * keep_h + g1 * ge_p;
        h2 = h2 * keep_h + g2 * ge_p;
        h3 = h3 * keep_h + g3 * ge_p;
        h4 = h4 * keep_h + g4 * ge_p;

        // Pack 5x 26-bit limbs into 4x 32-bit words and add pad s mod 2^128
        let f0: u64 = (h0 as u64) | (((h1 & 0x3F) as u64) << 26);
        let f1: u64 = ((h1 >> 6) as u64) | (((h2 & 0xFFF) as u64) << 20);
        let f2: u64 = ((h2 >> 12) as u64) | (((h3 & 0x3FFFF) as u64) << 14);
        let f3: u64 = ((h3 >> 18) as u64) | (((h4 & 0xFFFFFF) as u64) << 8);

        let w0: u64 = f0 + (_.pad0 as u64);
        let w1: u64 = f1 + (_.pad1 as u64) + (w0 >> 32);
        let w2: u64 = f2 + (_.pad2 as u64) + (w1 >> 32);
        let w3: u64 = f3 + (_.pad3 as u64) + (w2 >> 32);

        p1305WriteU32Le(out_tag16, 0, (w0 & 0xFFFFFFFF) as u32);
        p1305WriteU32Le(out_tag16, 4, (w1 & 0xFFFFFFFF) as u32);
        p1305WriteU32Le(out_tag16, 8, (w2 & 0xFFFFFFFF) as u32);
        p1305WriteU32Le(out_tag16, 12, (w3 & 0xFFFFFFFF) as u32);
    }
}

pub fn poly1305Mac(key32: []u8, msg: []u8, out_tag16: []u8): bool {
    if (key32.len != 32 || out_tag16.len < 16) {
        return false;
    }
    let st = Poly1305.init(key32);
    if (msg.len > 0) {
        st.update(msg);
    }
    st.finalize(out_tag16[0..16]);
    return true;
}

pub fn poly1305Verify(key32: []u8, msg: []u8, expected_tag16: []u8): bool {
    if (key32.len != 32 || expected_tag16.len != 16) {
        return false;
    }
    let computed: [16]u8 = undefined;
    if (!poly1305Mac(key32, msg, computed[0..16])) {
        return false;
    }
    return ctEqBytes(computed[0..16], expected_tag16).toBool();
}

export {
    POLY1305_KEY_LEN,
    POLY1305_TAG_LEN,
    Poly1305,
    poly1305Mac,
    poly1305Verify
};
