// @cap/tiny-keccak — Pure CP zero-allocation Keccak-f[1600] sponge (SHA3-256 & Keccak-256)
// Clean-room MIT parity with Rust crates.io `tiny-keccak` v2.
// Accelerated with 16-lane @Vector(16, u8) SIMD hex encoding, comptime round/LUT tables, and zero-copy bulk @memcpy/@memset.

pub const KECCAK256_DIGEST_LEN: usize = 32;
pub const KECCAK256_RATE: usize = 136;

type KckVec16 = @Vector(16, u8);

const KCK_ROUND_CONSTS: [24]u64 = [
    0x0000000000000001 as u64, 0x0000000000008082 as u64,
    0x800000000000808A as u64, 0x8000000080008000 as u64,
    0x000000000000808B as u64, 0x0000000080000001 as u64,
    0x8000000080008081 as u64, 0x8000000000008009 as u64,
    0x000000000000008A as u64, 0x0000000000000088 as u64,
    0x0000000080008009 as u64, 0x000000008000000A as u64,
    0x000000008000808B as u64, 0x800000000000008B as u64,
    0x8000000000008089 as u64, 0x8000000000008003 as u64,
    0x8000000000008002 as u64, 0x8000000000000080 as u64,
    0x000000000000800A as u64, 0x800000008000000A as u64,
    0x8000000080008081 as u64, 0x8000000000008080 as u64,
    0x0000000080000001 as u64, 0x8000000080008008 as u64,
];

const KCK_PILN: [24]usize = [
    10 as usize, 7 as usize, 11 as usize, 17 as usize, 18 as usize, 3 as usize,
    5 as usize, 16 as usize, 8 as usize, 21 as usize, 24 as usize, 4 as usize,
    15 as usize, 23 as usize, 19 as usize, 13 as usize, 12 as usize, 2 as usize,
    20 as usize, 14 as usize, 22 as usize, 9 as usize, 6 as usize, 1 as usize,
];

const KCK_ROTC: [24]u32 = [
    1 as u32, 3 as u32, 6 as u32, 10 as u32, 15 as u32, 21 as u32,
    28 as u32, 36 as u32, 45 as u32, 55 as u32, 2 as u32, 14 as u32,
    27 as u32, 41 as u32, 56 as u32, 8 as u32, 25 as u32, 43 as u32,
    62 as u32, 18 as u32, 39 as u32, 61 as u32, 20 as u32, 44 as u32,
];

fn kckRotl64(x: u64, n: u32): u64 {
    let r: u32 = n & (63 as u32);
    if (r == (0 as u32)) {
        return x;
    }
    const s1 = r as u6;
    const s2 = ((64 as u32) - r) as u6;
    return (x << s1) | (x >> s2);
}

fn kckReadLe64(b: []const u8, off: usize): u64 {
    const lo: u64 = (b[off] as u64)
        | ((b[off + (1 as usize)] as u64) << 8)
        | ((b[off + (2 as usize)] as u64) << 16)
        | ((b[off + (3 as usize)] as u64) << 24);
    const hi: u64 = (b[off + (4 as usize)] as u64)
        | ((b[off + (5 as usize)] as u64) << 8)
        | ((b[off + (6 as usize)] as u64) << 16)
        | ((b[off + (7 as usize)] as u64) << 24);
    return (hi << 32) | lo;
}

fn kckWriteLe64(out: []u8, off: usize, v: u64) {
    out[off] = (v & (0xFF as u64)) as u8;
    out[off + (1 as usize)] = ((v >> 8) & (0xFF as u64)) as u8;
    out[off + (2 as usize)] = ((v >> 16) & (0xFF as u64)) as u8;
    out[off + (3 as usize)] = ((v >> 24) & (0xFF as u64)) as u8;
    out[off + (4 as usize)] = ((v >> 32) & (0xFF as u64)) as u8;
    out[off + (5 as usize)] = ((v >> 40) & (0xFF as u64)) as u8;
    out[off + (6 as usize)] = ((v >> 48) & (0xFF as u64)) as u8;
    out[off + (7 as usize)] = ((v >> 56) & (0xFF as u64)) as u8;
}

fn kckPermute(st: []u64) {
    const all_ones: u64 = 0xFFFFFFFFFFFFFFFF as u64;
    let bc: [5]u64 = undefined;

    let round: usize = 0;
    while (round < (24 as usize)) {
        // Theta
        let x: usize = 0;
        while (x < (5 as usize)) {
            bc[x] = st[x] ^ st[x + (5 as usize)] ^ st[x + (10 as usize)] ^ st[x + (15 as usize)] ^ st[x + (20 as usize)];
            x += 1;
        }
        x = 0;
        while (x < (5 as usize)) {
            const t: u64 = bc[(x + (4 as usize)) % (5 as usize)] ^ kckRotl64(bc[(x + (1 as usize)) % (5 as usize)], 1 as u32);
            let y: usize = 0;
            while (y < (25 as usize)) {
                st[y + x] = st[y + x] ^ t;
                y += 5 as usize;
            }
            x += 1;
        }

        // Rho & Pi
        let cur: u64 = st[1];
        let i: usize = 0;
        while (i < (24 as usize)) {
            const j: usize = KCK_PILN[i];
            const tmp: u64 = st[j];
            st[j] = kckRotl64(cur, KCK_ROTC[i]);
            cur = tmp;
            i += 1;
        }

        // Chi
        let row: usize = 0;
        while (row < (25 as usize)) {
            bc[0] = st[row];
            bc[1] = st[row + 1];
            bc[2] = st[row + 2];
            bc[3] = st[row + 3];
            bc[4] = st[row + 4];
            st[row] = bc[0] ^ ((bc[1] ^ all_ones) & bc[2]);
            st[row + 1] = bc[1] ^ ((bc[2] ^ all_ones) & bc[3]);
            st[row + 2] = bc[2] ^ ((bc[3] ^ all_ones) & bc[4]);
            st[row + 3] = bc[3] ^ ((bc[4] ^ all_ones) & bc[0]);
            st[row + 4] = bc[4] ^ ((bc[0] ^ all_ones) & bc[1]);
            row += 5 as usize;
        }

        // Iota
        st[0] = st[0] ^ KCK_ROUND_CONSTS[round];
        round += 1;
    }
}

fn kckAbsorbBlock(st: []u64, block: []const u8) {
    let i: usize = 0;
    while (i < (17 as usize)) {
        st[i] = st[i] ^ kckReadLe64(block, i * (8 as usize));
        i += 1;
    }
    kckPermute(st);
}

fn kckAbsorbSlice(st: []u64, buf: []u8, buf_len: usize, data: []const u8): usize {
    let blen = buf_len;
    let i: usize = 0;
    if (blen > 0) {
        let take = KECCAK256_RATE - blen;
        if (take > data.len) {
            take = data.len;
        }
        if (take > 0) {
            @memcpy(buf[blen .. blen + take], data[0..take]);
            blen += take;
            i += take;
        }
        if (blen == KECCAK256_RATE) {
            kckAbsorbBlock(st, buf[0..136]);
            blen = 0;
        }
    }
    while (i + KECCAK256_RATE <= data.len) {
        kckAbsorbBlock(st, data[i .. i + KECCAK256_RATE]);
        i += KECCAK256_RATE;
    }
    const rem = data.len - i;
    if (rem > 0) {
        @memcpy(buf[blen .. blen + rem], data[i..data.len]);
        blen += rem;
    }
    return blen;
}

fn kckFinalizeSponge(st: []u64, buf: []u8, buf_len: usize, domain_sep: u8, out32: []u8) {
    if (out32.len < KECCAK256_DIGEST_LEN) {
        return;
    }
    @memset(buf[buf_len..KECCAK256_RATE], 0);
    buf[buf_len] = domain_sep;
    buf[KECCAK256_RATE - (1 as usize)] = buf[KECCAK256_RATE - (1 as usize)] | (0x80 as u8);
    kckAbsorbBlock(st, buf);

    kckWriteLe64(out32, 0 as usize, st[0]);
    kckWriteLe64(out32, 8 as usize, st[1]);
    kckWriteLe64(out32, 16 as usize, st[2]);
    kckWriteLe64(out32, 24 as usize, st[3]);
}

// 16-lane SIMD branchless hex encoding for 16 bytes -> 32 ASCII hex bytes
fn kckEncodeHex16Simd(src: []const u8, dst: []u8) {
    const v: KckVec16 = src[0..16].*;
    const v_mask: KckVec16 = @splat(0x0F as u8);
    const v_ten: KckVec16 = @splat(10 as u8);
    const v_48: KckVec16 = @splat(48 as u8);
    const v_87: KckVec16 = @splat(87 as u8);
    const v_shift: @Vector(16, u3) = @splat(4 as u3);

    const hi_nib: KckVec16 = v >> v_shift;
    const lo_nib: KckVec16 = v & v_mask;
    const hi_ascii: [16]u8 = @select(u8, hi_nib < v_ten, hi_nib + v_48, hi_nib + v_87);
    const lo_ascii: [16]u8 = @select(u8, lo_nib < v_ten, lo_nib + v_48, lo_nib + v_87);

    let i: usize = 0;
    while (i < 16) {
        dst[i * 2] = hi_ascii[i];
        dst[i * 2 + 1] = lo_ascii[i];
        i += 1;
    }
}

fn kckEncodeHex32(digest: []const u8, out64: []u8): usize {
    if (out64.len < (64 as usize)) {
        return 0;
    }
    kckEncodeHex16Simd(digest[0..16], out64[0..32]);
    kckEncodeHex16Simd(digest[16..32], out64[32..64]);
    return 64;
}

pub struct Sha3_256 {
    st: [25]u64,
    buf: [136]u8,
    buf_len: usize,

    pub static fn new(): Sha3_256 {
        let st_arr: [25]u64 = undefined;
        @memset(st_arr[0..25], 0 as u64);
        let buf_arr: [136]u8 = undefined;
        @memset(buf_arr[0..KECCAK256_RATE], 0);
        return Sha3_256 {
            st: st_arr,
            buf: buf_arr,
            buf_len: 0,
        };
    }

    pub fn update(*_, data: []u8) {
        _.buf_len = kckAbsorbSlice(_.st[0..25], _.buf[0..136], _.buf_len, data);
    }

    pub fn updateStr(*_, s: string) {
        _.buf_len = kckAbsorbSlice(_.st[0..25], _.buf[0..136], _.buf_len, s);
    }

    pub fn finalize(*_, out32: []u8) {
        kckFinalizeSponge(_.st[0..25], _.buf[0..136], _.buf_len, 0x06 as u8, out32);
        _.buf_len = 0;
    }

    pub fn finalizeHex(*_, out64: []u8): usize {
        if (out64.len < (64 as usize)) {
            return 0;
        }
        let digest: [32]u8 = undefined;
        _.finalize(digest[0..32]);
        return kckEncodeHex32(digest[0..32], out64);
    }
}

pub struct Keccak256 {
    st: [25]u64,
    buf: [136]u8,
    buf_len: usize,

    pub static fn new(): Keccak256 {
        let st_arr: [25]u64 = undefined;
        @memset(st_arr[0..25], 0 as u64);
        let buf_arr: [136]u8 = undefined;
        @memset(buf_arr[0..KECCAK256_RATE], 0);
        return Keccak256 {
            st: st_arr,
            buf: buf_arr,
            buf_len: 0,
        };
    }

    pub fn update(*_, data: []u8) {
        _.buf_len = kckAbsorbSlice(_.st[0..25], _.buf[0..136], _.buf_len, data);
    }

    pub fn updateStr(*_, s: string) {
        _.buf_len = kckAbsorbSlice(_.st[0..25], _.buf[0..136], _.buf_len, s);
    }

    pub fn finalize(*_, out32: []u8) {
        kckFinalizeSponge(_.st[0..25], _.buf[0..136], _.buf_len, 0x01 as u8, out32);
        _.buf_len = 0;
    }

    pub fn finalizeHex(*_, out64: []u8): usize {
        if (out64.len < (64 as usize)) {
            return 0;
        }
        let digest: [32]u8 = undefined;
        _.finalize(digest[0..32]);
        return kckEncodeHex32(digest[0..32], out64);
    }
}

pub fn sha3_256Hex(s: string, out64: []u8): usize {
    let h = Sha3_256.new();
    h.updateStr(s);
    return h.finalizeHex(out64);
}

pub fn keccak256Hex(s: string, out64: []u8): usize {
    let h = Keccak256.new();
    h.updateStr(s);
    return h.finalizeHex(out64);
}
