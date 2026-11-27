// @cap/tiny-keccak — Pure CP zero-allocation Keccak-f[1600] sponge (SHA3-256 & Keccak-256)
// Clean-room MIT parity with Rust crates.io `tiny-keccak` v2.

pub const KECCAK256_DIGEST_LEN: usize = 32;
pub const KECCAK256_RATE: usize = 136;

fn kckU64(hi: u32, lo: u32): u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn kckRotl64(x: u64, n: u32): u64 {
    let r: u32 = n & (63 as u32);
    if (r == (0 as u32)) {
        return x;
    }
    let v: u64 = x;
    if ((r & (1 as u32)) != (0 as u32)) { v = (v << 1) | (v >> 63); }
    if ((r & (2 as u32)) != (0 as u32)) { v = (v << 2) | (v >> 62); }
    if ((r & (4 as u32)) != (0 as u32)) { v = (v << 4) | (v >> 60); }
    if ((r & (8 as u32)) != (0 as u32)) { v = (v << 8) | (v >> 56); }
    if ((r & (16 as u32)) != (0 as u32)) { v = (v << 16) | (v >> 48); }
    if ((r & (32 as u32)) != (0 as u32)) { v = (v << 32) | (v >> 32); }
    return v;
}

fn kckReadLe64(b: []u8, off: usize): u64 {
    let lo: u32 = (b[off] as u32)
        | ((b[off + (1 as usize)] as u32) << 8)
        | ((b[off + (2 as usize)] as u32) << 16)
        | ((b[off + (3 as usize)] as u32) << 24);
    let hi: u32 = (b[off + (4 as usize)] as u32)
        | ((b[off + (5 as usize)] as u32) << 8)
        | ((b[off + (6 as usize)] as u32) << 16)
        | ((b[off + (7 as usize)] as u32) << 24);
    return kckU64(hi, lo);
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

fn kckHexDigit(nibble: u8): u8 {
    if (nibble < (10 as u8)) {
        return ('0' as u8) + nibble;
    }
    return ('a' as u8) + (nibble - (10 as u8));
}

fn kckRoundConst(round: usize): u64 {
    if (round == (0 as usize)) { return kckU64(0x00000000 as u32, 0x00000001 as u32); }
    if (round == (1 as usize)) { return kckU64(0x00000000 as u32, 0x00008082 as u32); }
    if (round == (2 as usize)) { return kckU64(0x80000000 as u32, 0x0000808A as u32); }
    if (round == (3 as usize)) { return kckU64(0x80000000 as u32, 0x80008000 as u32); }
    if (round == (4 as usize)) { return kckU64(0x00000000 as u32, 0x0000808B as u32); }
    if (round == (5 as usize)) { return kckU64(0x00000000 as u32, 0x80000001 as u32); }
    if (round == (6 as usize)) { return kckU64(0x80000000 as u32, 0x80008081 as u32); }
    if (round == (7 as usize)) { return kckU64(0x80000000 as u32, 0x00008009 as u32); }
    if (round == (8 as usize)) { return kckU64(0x00000000 as u32, 0x0000008A as u32); }
    if (round == (9 as usize)) { return kckU64(0x00000000 as u32, 0x00000088 as u32); }
    if (round == (10 as usize)) { return kckU64(0x00000000 as u32, 0x80008009 as u32); }
    if (round == (11 as usize)) { return kckU64(0x00000000 as u32, 0x8000000A as u32); }
    if (round == (12 as usize)) { return kckU64(0x00000000 as u32, 0x8000808B as u32); }
    if (round == (13 as usize)) { return kckU64(0x80000000 as u32, 0x0000008B as u32); }
    if (round == (14 as usize)) { return kckU64(0x80000000 as u32, 0x00008089 as u32); }
    if (round == (15 as usize)) { return kckU64(0x80000000 as u32, 0x00008003 as u32); }
    if (round == (16 as usize)) { return kckU64(0x80000000 as u32, 0x00008002 as u32); }
    if (round == (17 as usize)) { return kckU64(0x80000000 as u32, 0x00000080 as u32); }
    if (round == (18 as usize)) { return kckU64(0x00000000 as u32, 0x0000800A as u32); }
    if (round == (19 as usize)) { return kckU64(0x80000000 as u32, 0x8000000A as u32); }
    if (round == (20 as usize)) { return kckU64(0x80000000 as u32, 0x80008081 as u32); }
    if (round == (21 as usize)) { return kckU64(0x80000000 as u32, 0x00008080 as u32); }
    if (round == (22 as usize)) { return kckU64(0x00000000 as u32, 0x80000001 as u32); }
    return kckU64(0x80000000 as u32, 0x80008008 as u32);
}

fn kckPermute(st: []u64) {
    let piln: [24]usize = [
        10 as usize, 7 as usize, 11 as usize, 17 as usize, 18 as usize, 3 as usize,
        5 as usize, 16 as usize, 8 as usize, 21 as usize, 24 as usize, 4 as usize,
        15 as usize, 23 as usize, 19 as usize, 13 as usize, 12 as usize, 2 as usize,
        20 as usize, 14 as usize, 22 as usize, 9 as usize, 6 as usize, 1 as usize,
    ];
    let rotc: [24]u32 = [
        1 as u32, 3 as u32, 6 as u32, 10 as u32, 15 as u32, 21 as u32,
        28 as u32, 36 as u32, 45 as u32, 55 as u32, 2 as u32, 14 as u32,
        27 as u32, 41 as u32, 56 as u32, 8 as u32, 25 as u32, 43 as u32,
        62 as u32, 18 as u32, 39 as u32, 61 as u32, 20 as u32, 44 as u32,
    ];
    let all_ones: u64 = kckU64(0xFFFFFFFF as u32, 0xFFFFFFFF as u32);
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
            let t: u64 = bc[(x + (4 as usize)) % (5 as usize)] ^ kckRotl64(bc[(x + (1 as usize)) % (5 as usize)], 1 as u32);
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
            let j: usize = piln[i];
            let tmp: u64 = st[j];
            st[j] = kckRotl64(cur, rotc[i]);
            cur = tmp;
            i += 1;
        }

        // Chi
        let row: usize = 0;
        while (row < (25 as usize)) {
            x = 0;
            while (x < (5 as usize)) {
                bc[x] = st[row + x];
                x += 1;
            }
            x = 0;
            while (x < (5 as usize)) {
                let not_next: u64 = bc[(x + (1 as usize)) % (5 as usize)] ^ all_ones;
                st[row + x] = bc[x] ^ (not_next & bc[(x + (2 as usize)) % (5 as usize)]);
                x += 1;
            }
            row += 5 as usize;
        }

        // Iota
        st[0] = st[0] ^ kckRoundConst(round);
        round += 1;
    }
}

fn kckAbsorbBlock(st: []u64, block: []u8) {
    let i: usize = 0;
    while (i < (17 as usize)) {
        st[i] = st[i] ^ kckReadLe64(block, i * (8 as usize));
        i += 1;
    }
    kckPermute(st);
}

fn kckFinalizeSponge(st: []u64, buf: []u8, buf_len: usize, domain_sep: u8, out32: []u8) {
    if (out32.len < KECCAK256_DIGEST_LEN) {
        return;
    }
    let i: usize = buf_len;
    while (i < KECCAK256_RATE) {
        buf[i] = 0;
        i += 1;
    }
    buf[buf_len] = domain_sep;
    buf[KECCAK256_RATE - (1 as usize)] = buf[KECCAK256_RATE - (1 as usize)] | (0x80 as u8);
    kckAbsorbBlock(st, buf);

    kckWriteLe64(out32, 0 as usize, st[0]);
    kckWriteLe64(out32, 8 as usize, st[1]);
    kckWriteLe64(out32, 16 as usize, st[2]);
    kckWriteLe64(out32, 24 as usize, st[3]);
}

fn kckEncodeHex32(digest: []u8, out64: []u8): usize {
    if (out64.len < (64 as usize)) {
        return 0;
    }
    let i: usize = 0;
    while (i < KECCAK256_DIGEST_LEN) {
        let b: u8 = digest[i];
        out64[i * (2 as usize)] = kckHexDigit((b >> 4) & (0x0F as u8));
        out64[i * (2 as usize) + (1 as usize)] = kckHexDigit(b & (0x0F as u8));
        i += 1;
    }
    return 64;
}

pub struct Sha3_256 {
    st: [25]u64,
    buf: [136]u8,
    buf_len: usize,

    pub static fn new(): Sha3_256 {
        let st_arr: [25]u64 = undefined;
        let i: usize = 0;
        while (i < (25 as usize)) {
            st_arr[i] = 0 as u64;
            i += 1;
        }
        let buf_arr: [136]u8 = undefined;
        i = 0;
        while (i < KECCAK256_RATE) {
            buf_arr[i] = 0;
            i += 1;
        }
        return Sha3_256 {
            st: st_arr,
            buf: buf_arr,
            buf_len: 0,
        };
    }

    pub fn update(*_, data: []u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            if (_.buf_len == KECCAK256_RATE) {
                kckAbsorbBlock(_.st[0..25], _.buf[0..136]);
                _.buf_len = 0;
            }
            i += 1;
        }
    }

    pub fn updateStr(*_, s: string) {
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.buf_len] = s[i];
            _.buf_len += 1;
            if (_.buf_len == KECCAK256_RATE) {
                kckAbsorbBlock(_.st[0..25], _.buf[0..136]);
                _.buf_len = 0;
            }
            i += 1;
        }
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
        let i: usize = 0;
        while (i < (25 as usize)) {
            st_arr[i] = 0 as u64;
            i += 1;
        }
        let buf_arr: [136]u8 = undefined;
        i = 0;
        while (i < KECCAK256_RATE) {
            buf_arr[i] = 0;
            i += 1;
        }
        return Keccak256 {
            st: st_arr,
            buf: buf_arr,
            buf_len: 0,
        };
    }

    pub fn update(*_, data: []u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            if (_.buf_len == KECCAK256_RATE) {
                kckAbsorbBlock(_.st[0..25], _.buf[0..136]);
                _.buf_len = 0;
            }
            i += 1;
        }
    }

    pub fn updateStr(*_, s: string) {
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.buf_len] = s[i];
            _.buf_len += 1;
            if (_.buf_len == KECCAK256_RATE) {
                kckAbsorbBlock(_.st[0..25], _.buf[0..136]);
                _.buf_len = 0;
            }
            i += 1;
        }
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
