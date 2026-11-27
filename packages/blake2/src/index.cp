// @cap/blake2-crate — Pure CP zero-allocation RFC 7693 BLAKE2s-256 (unkeyed & keyed MAC)
// Clean-room MIT parity with Rust crates.io `blake2` v0.10.
// Accelerated with 16-lane @Vector(16, u8) SIMD hex encoding, @Vector(4, u32) state XOR, comptime LUTs, and bulk @memcpy/@memset.

pub const BLAKE2S_DIGEST_LEN: usize = 32;
pub const BLAKE2S_BLOCK_LEN: usize = 64;

type Bl2sVec16 = @Vector(16, u8);
type Bl2sVec4U32 = @Vector(4, u32);

const BL2S_IV: [8]u32 = [
    0x6A09E667 as u32, 0xBB67AE85 as u32, 0x3C6EF372 as u32, 0xA54FF53A as u32,
    0x510E527F as u32, 0x9B05688C as u32, 0x1F83D9AB as u32, 0x5BE0CD19 as u32,
];

const BL2S_SIGMA: [160]usize = [
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

fn bl2sReadLe32(b: []const u8, off: usize): u32 {
    const b0: u32 = b[off] as u32;
    const b1: u32 = (b[off + (1 as usize)] as u32) << 8;
    const b2: u32 = (b[off + (2 as usize)] as u32) << 16;
    const b3: u32 = (b[off + (3 as usize)] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

fn bl2sWriteLe32(out: []u8, off: usize, v: u32) {
    out[off] = (v & (0xFF as u32)) as u8;
    out[off + (1 as usize)] = ((v >> 8) & (0xFF as u32)) as u8;
    out[off + (2 as usize)] = ((v >> 16) & (0xFF as u32)) as u8;
    out[off + (3 as usize)] = ((v >> 24) & (0xFF as u32)) as u8;
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

fn bl2sCompress(h: []u32, block: []const u8, t: u64, is_last: bool) {
    let m: [16]u32 = undefined;
    let i: usize = 0;
    while (i < (16 as usize)) {
        m[i] = bl2sReadLe32(block, i * (4 as usize));
        i += 1;
    }

    let v: [16]u32 = undefined;
    @memcpy(v[0..8], h[0..8]);
    @memcpy(v[8..16], BL2S_IV[0..8]);

    v[12] = v[12] ^ ((t & (0xFFFFFFFF as u64)) as u32);
    v[13] = v[13] ^ (((t >> 32) & (0xFFFFFFFF as u64)) as u32);
    if (is_last) {
        v[14] = v[14] ^ (0xFFFFFFFF as u32);
    }

    let r: usize = 0;
    while (r < (10 as usize)) {
        const base: usize = r * (16 as usize);
        bl2sMix(v[0..16], 0 as usize, 4 as usize, 8 as usize, 12 as usize, m[BL2S_SIGMA[base + (0 as usize)]], m[BL2S_SIGMA[base + (1 as usize)]]);
        bl2sMix(v[0..16], 1 as usize, 5 as usize, 9 as usize, 13 as usize, m[BL2S_SIGMA[base + (2 as usize)]], m[BL2S_SIGMA[base + (3 as usize)]]);
        bl2sMix(v[0..16], 2 as usize, 6 as usize, 10 as usize, 14 as usize, m[BL2S_SIGMA[base + (4 as usize)]], m[BL2S_SIGMA[base + (5 as usize)]]);
        bl2sMix(v[0..16], 3 as usize, 7 as usize, 11 as usize, 15 as usize, m[BL2S_SIGMA[base + (6 as usize)]], m[BL2S_SIGMA[base + (7 as usize)]]);

        bl2sMix(v[0..16], 0 as usize, 5 as usize, 10 as usize, 15 as usize, m[BL2S_SIGMA[base + (8 as usize)]], m[BL2S_SIGMA[base + (9 as usize)]]);
        bl2sMix(v[0..16], 1 as usize, 6 as usize, 11 as usize, 12 as usize, m[BL2S_SIGMA[base + (10 as usize)]], m[BL2S_SIGMA[base + (11 as usize)]]);
        bl2sMix(v[0..16], 2 as usize, 7 as usize, 8 as usize, 13 as usize, m[BL2S_SIGMA[base + (12 as usize)]], m[BL2S_SIGMA[base + (13 as usize)]]);
        bl2sMix(v[0..16], 3 as usize, 4 as usize, 9 as usize, 14 as usize, m[BL2S_SIGMA[base + (14 as usize)]], m[BL2S_SIGMA[base + (15 as usize)]]);
        r += 1;
    }

    const vh0: Bl2sVec4U32 = h[0..4].*;
    const vv0: Bl2sVec4U32 = v[0..4].*;
    const vv8: Bl2sVec4U32 = v[8..12].*;
    const out0: [4]u32 = vh0 ^ vv0 ^ vv8;
    @memcpy(h[0..4], out0[0..4]);

    const vh1: Bl2sVec4U32 = h[4..8].*;
    const vv1: Bl2sVec4U32 = v[4..8].*;
    const vv12: Bl2sVec4U32 = v[12..16].*;
    const out1: [4]u32 = vh1 ^ vv1 ^ vv12;
    @memcpy(h[4..8], out1[0..4]);
}

// 16-lane SIMD branchless hex encoding for 16 bytes -> 32 ASCII hex bytes
fn bl2sEncodeHex16Simd(src: []const u8, dst: []u8) {
    const v: Bl2sVec16 = src[0..16].*;
    const v_mask: Bl2sVec16 = @splat(0x0F as u8);
    const v_ten: Bl2sVec16 = @splat(10 as u8);
    const v_48: Bl2sVec16 = @splat(48 as u8);
    const v_87: Bl2sVec16 = @splat(87 as u8);
    const v_shift: @Vector(16, u3) = @splat(4 as u3);

    const hi_nib: Bl2sVec16 = v >> v_shift;
    const lo_nib: Bl2sVec16 = v & v_mask;
    const hi_ascii: [16]u8 = @select(u8, hi_nib < v_ten, hi_nib + v_48, hi_nib + v_87);
    const lo_ascii: [16]u8 = @select(u8, lo_nib < v_ten, lo_nib + v_48, lo_nib + v_87);

    let i: usize = 0;
    while (i < 16) {
        dst[i * 2] = hi_ascii[i];
        dst[i * 2 + 1] = lo_ascii[i];
        i += 1;
    }
}

pub struct Blake2s256 {
    h: [8]u32,
    t: u64,
    buf: [64]u8,
    buf_len: usize,

    pub static fn new(): Blake2s256 {
        let h_arr: [8]u32 = BL2S_IV;
        // fanout=1, depth=1, key_len=0, digest_len=32 -> 0x01010020
        h_arr[0] = h_arr[0] ^ (0x01010020 as u32);

        let b_arr: [64]u8 = undefined;
        @memset(b_arr[0..BLAKE2S_BLOCK_LEN], 0);
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
        let h_arr: [8]u32 = BL2S_IV;
        const param0: u32 = (0x01010000 as u32) | (((key.len as u32) & (0xFF as u32)) << 8) | (32 as u32);
        h_arr[0] = h_arr[0] ^ param0;

        let b_arr: [64]u8 = undefined;
        @memset(b_arr[0..BLAKE2S_BLOCK_LEN], 0);
        let initial_len: usize = 0;
        if (key.len > (0 as usize)) {
            @memcpy(b_arr[0..key.len], key[0..key.len]);
            initial_len = BLAKE2S_BLOCK_LEN;
        }
        return Blake2s256 {
            h: h_arr,
            t: 0 as u64,
            buf: b_arr,
            buf_len: initial_len,
        };
    }

    fn updateSlice(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) {
            if (_.buf_len == BLAKE2S_BLOCK_LEN) {
                _.t += BLAKE2S_BLOCK_LEN as u64;
                bl2sCompress(_.h[0..8], _.buf[0..64], _.t, false);
                _.buf_len = 0;
            }
            const rem = data.len - i;
            if (_.buf_len == 0 && rem > BLAKE2S_BLOCK_LEN) {
                _.t += BLAKE2S_BLOCK_LEN as u64;
                bl2sCompress(_.h[0..8], data[i .. i + BLAKE2S_BLOCK_LEN], _.t, false);
                i += BLAKE2S_BLOCK_LEN;
                continue;
            }
            let take = BLAKE2S_BLOCK_LEN - _.buf_len;
            if (take > rem) {
                take = rem;
            }
            @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[i .. i + take]);
            _.buf_len += take;
            i += take;
        }
    }

    pub fn update(*_, data: []u8) {
        _.updateSlice(data);
    }

    pub fn updateStr(*_, s: string) {
        _.updateSlice(s);
    }

    pub fn finalize(*_, out32: []u8) {
        if (out32.len < BLAKE2S_DIGEST_LEN) {
            return;
        }
        _.t += _.buf_len as u64;
        @memset(_.buf[_.buf_len..BLAKE2S_BLOCK_LEN], 0);
        bl2sCompress(_.h[0..8], _.buf[0..64], _.t, true);
        _.buf_len = 0;

        let i: usize = 0;
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
        bl2sEncodeHex16Simd(digest[0..16], out64[0..32]);
        bl2sEncodeHex16Simd(digest[16..32], out64[32..64]);
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
