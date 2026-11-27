// @cap/blake2-crate — Pure CP zero-allocation RFC 7693 BLAKE2s-256 (unkeyed & keyed MAC)
// Clean-room MIT parity with Rust crates.io `blake2` v0.10.

pub const BLAKE2S_DIGEST_LEN: usize = 32;
pub const BLAKE2S_BLOCK_LEN: usize = 64;

fn bl2sAdd2(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn bl2sAdd3(a: u32, b: u32, c: u32): u32 {
    return (((a as u64) + (b as u64) + (c as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn bl2sRotr16(x: u32): u32 {
    return (x >> 16) | (x << 16);
}

fn bl2sRotr12(x: u32): u32 {
    return (x >> 12) | (x << 20);
}

fn bl2sRotr8(x: u32): u32 {
    return (x >> 8) | (x << 24);
}

fn bl2sRotr7(x: u32): u32 {
    return (x >> 7) | (x << 25);
}

fn bl2sReadLe32(b: []u8, off: usize): u32 {
    let b0: u32 = b[off] as u32;
    let b1: u32 = (b[off + (1 as usize)] as u32) << 8;
    let b2: u32 = (b[off + (2 as usize)] as u32) << 16;
    let b3: u32 = (b[off + (3 as usize)] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

fn bl2sWriteLe32(out: []u8, off: usize, v: u32) {
    out[off] = (v & (0xFF as u32)) as u8;
    out[off + (1 as usize)] = ((v >> 8) & (0xFF as u32)) as u8;
    out[off + (2 as usize)] = ((v >> 16) & (0xFF as u32)) as u8;
    out[off + (3 as usize)] = ((v >> 24) & (0xFF as u32)) as u8;
}

fn bl2sHexDigit(nibble: u8): u8 {
    if (nibble < (10 as u8)) {
        return ('0' as u8) + nibble;
    }
    return ('a' as u8) + (nibble - (10 as u8));
}

fn bl2sIv(idx: usize): u32 {
    if (idx == (0 as usize)) { return 0x6A09E667 as u32; }
    if (idx == (1 as usize)) { return 0xBB67AE85 as u32; }
    if (idx == (2 as usize)) { return 0x3C6EF372 as u32; }
    if (idx == (3 as usize)) { return 0xA54FF53A as u32; }
    if (idx == (4 as usize)) { return 0x510E527F as u32; }
    if (idx == (5 as usize)) { return 0x9B05688C as u32; }
    if (idx == (6 as usize)) { return 0x1F83D9AB as u32; }
    return 0x5BE0CD19 as u32;
}

fn bl2sMix(v: []u32, ai: usize, bi: usize, ci: usize, di: usize, x: u32, y: u32) {
    let a: u32 = v[ai];
    let b: u32 = v[bi];
    let c: u32 = v[ci];
    let d: u32 = v[di];

    a = bl2sAdd3(a, b, x);
    d = bl2sRotr16(d ^ a);
    c = bl2sAdd2(c, d);
    b = bl2sRotr12(b ^ c);
    a = bl2sAdd3(a, b, y);
    d = bl2sRotr8(d ^ a);
    c = bl2sAdd2(c, d);
    b = bl2sRotr7(b ^ c);

    v[ai] = a;
    v[bi] = b;
    v[ci] = c;
    v[di] = d;
}

fn bl2sCompress(h: []u32, block: []u8, t: u64, is_last: bool) {
    let sigma: [160]usize = [
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3,
        11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4,
        7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8,
        9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13,
        2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9,
        12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11,
        13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10,
        6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5,
        10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0,
    ];

    let m: [16]u32 = undefined;
    let i: usize = 0;
    while (i < (16 as usize)) {
        m[i] = bl2sReadLe32(block, i * (4 as usize));
        i += 1;
    }

    let v: [16]u32 = undefined;
    i = 0;
    while (i < (8 as usize)) {
        v[i] = h[i];
        v[i + (8 as usize)] = bl2sIv(i);
        i += 1;
    }

    v[12] = v[12] ^ ((t & (0xFFFFFFFF as u64)) as u32);
    v[13] = v[13] ^ (((t >> 32) & (0xFFFFFFFF as u64)) as u32);
    if (is_last) {
        v[14] = v[14] ^ (0xFFFFFFFF as u32);
    }

    let r: usize = 0;
    while (r < (10 as usize)) {
        let base: usize = r * (16 as usize);
        bl2sMix(v[0..16], 0 as usize, 4 as usize, 8 as usize, 12 as usize, m[sigma[base + (0 as usize)]], m[sigma[base + (1 as usize)]]);
        bl2sMix(v[0..16], 1 as usize, 5 as usize, 9 as usize, 13 as usize, m[sigma[base + (2 as usize)]], m[sigma[base + (3 as usize)]]);
        bl2sMix(v[0..16], 2 as usize, 6 as usize, 10 as usize, 14 as usize, m[sigma[base + (4 as usize)]], m[sigma[base + (5 as usize)]]);
        bl2sMix(v[0..16], 3 as usize, 7 as usize, 11 as usize, 15 as usize, m[sigma[base + (6 as usize)]], m[sigma[base + (7 as usize)]]);

        bl2sMix(v[0..16], 0 as usize, 5 as usize, 10 as usize, 15 as usize, m[sigma[base + (8 as usize)]], m[sigma[base + (9 as usize)]]);
        bl2sMix(v[0..16], 1 as usize, 6 as usize, 11 as usize, 12 as usize, m[sigma[base + (10 as usize)]], m[sigma[base + (11 as usize)]]);
        bl2sMix(v[0..16], 2 as usize, 7 as usize, 8 as usize, 13 as usize, m[sigma[base + (12 as usize)]], m[sigma[base + (13 as usize)]]);
        bl2sMix(v[0..16], 3 as usize, 4 as usize, 9 as usize, 14 as usize, m[sigma[base + (14 as usize)]], m[sigma[base + (15 as usize)]]);
        r += 1;
    }

    i = 0;
    while (i < (8 as usize)) {
        h[i] = h[i] ^ v[i] ^ v[i + (8 as usize)];
        i += 1;
    }
}

pub struct Blake2s256 {
    h: [8]u32,
    t: u64,
    buf: [64]u8,
    buf_len: usize,

    pub static fn new(): Blake2s256 {
        let h_arr: [8]u32 = undefined;
        let i: usize = 0;
        while (i < (8 as usize)) {
            h_arr[i] = bl2sIv(i);
            i += 1;
        }
        // fanout=1, depth=1, key_len=0, digest_len=32 -> 0x01010020
        h_arr[0] = h_arr[0] ^ (0x01010020 as u32);

        let b_arr: [64]u8 = undefined;
        i = 0;
        while (i < BLAKE2S_BLOCK_LEN) {
            b_arr[i] = 0;
            i += 1;
        }
        return Blake2s256 {
            h: h_arr,
            t: 0 as u64,
            buf: b_arr,
            buf_len: 0,
        };
    }

    pub static fn newKeyed(key: []u8): Blake2s256? {
        if (key.len > (32 as usize)) {
            return null;
        }
        let h_arr: [8]u32 = undefined;
        let i: usize = 0;
        while (i < (8 as usize)) {
            h_arr[i] = bl2sIv(i);
            i += 1;
        }
        let param0: u32 = (0x01010000 as u32) | (((key.len as u32) & (0xFF as u32)) << 8) | (32 as u32);
        h_arr[0] = h_arr[0] ^ param0;

        let b_arr: [64]u8 = undefined;
        i = 0;
        while (i < BLAKE2S_BLOCK_LEN) {
            b_arr[i] = 0;
            i += 1;
        }
        let initial_len: usize = 0;
        if (key.len > (0 as usize)) {
            i = 0;
            while (i < key.len) {
                b_arr[i] = key[i];
                i += 1;
            }
            initial_len = BLAKE2S_BLOCK_LEN;
        }
        return Blake2s256 {
            h: h_arr,
            t: 0 as u64,
            buf: b_arr,
            buf_len: initial_len,
        };
    }

    pub fn update(*_, data: []u8) {
        let i: usize = 0;
        while (i < data.len) {
            if (_.buf_len == BLAKE2S_BLOCK_LEN) {
                _.t += BLAKE2S_BLOCK_LEN as u64;
                bl2sCompress(_.h[0..8], _.buf[0..64], _.t, false);
                _.buf_len = 0;
            }
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            i += 1;
        }
    }

    pub fn updateStr(*_, s: string) {
        let i: usize = 0;
        while (i < s.len) {
            if (_.buf_len == BLAKE2S_BLOCK_LEN) {
                _.t += BLAKE2S_BLOCK_LEN as u64;
                bl2sCompress(_.h[0..8], _.buf[0..64], _.t, false);
                _.buf_len = 0;
            }
            _.buf[_.buf_len] = s[i];
            _.buf_len += 1;
            i += 1;
        }
    }

    pub fn finalize(*_, out32: []u8) {
        if (out32.len < BLAKE2S_DIGEST_LEN) {
            return;
        }
        _.t += _.buf_len as u64;
        let i: usize = _.buf_len;
        while (i < BLAKE2S_BLOCK_LEN) {
            _.buf[i] = 0;
            i += 1;
        }
        bl2sCompress(_.h[0..8], _.buf[0..64], _.t, true);
        _.buf_len = 0;

        i = 0;
        while (i < (8 as usize)) {
            bl2sWriteLe32(out32, i * (4 as usize), _.h[i]);
            i += 1;
        }
    }

    pub fn finalizeHex(*_, out64: []u8): usize {
        if (out64.len < (64 as usize)) {
            return 0;
        }
        let digest: [32]u8 = undefined;
        _.finalize(digest[0..32]);
        let i: usize = 0;
        while (i < BLAKE2S_DIGEST_LEN) {
            let b: u8 = digest[i];
            out64[i * (2 as usize)] = bl2sHexDigit((b >> 4) & (0x0F as u8));
            out64[i * (2 as usize) + (1 as usize)] = bl2sHexDigit(b & (0x0F as u8));
            i += 1;
        }
        return 64;
    }
}

pub fn blake2s256Bytes(data: []u8, out32: []u8) {
    let h = Blake2s256.new();
    h.update(data);
    h.finalize(out32);
}

pub fn blake2s256Hex(s: string, out64: []u8): usize {
    let h = Blake2s256.new();
    h.updateStr(s);
    return h.finalizeHex(out64);
}
