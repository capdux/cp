// @cap/md5-crate — Pure CP zero-allocation RFC 1321 MD5 implementation
// Clean-room MIT parity with Rust crates.io `md-5` / `md5`.

pub const MD5_DIGEST_LEN: usize = 16;
pub const MD5_BLOCK_LEN: usize = 64;

fn md5Add32(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn md5Add4(a: u32, b: u32, c: u32, d: u32): u32 {
    let sum: u64 = (a as u64) + (b as u64) + (c as u64) + (d as u64);
    return (sum & (0xFFFFFFFF as u64)) as u32;
}

fn md5Rotl32(x: u32, bits: u32): u32 {
    if (bits == (4 as u32)) { return (x << 4) | (x >> 28); }
    if (bits == (5 as u32)) { return (x << 5) | (x >> 27); }
    if (bits == (6 as u32)) { return (x << 6) | (x >> 26); }
    if (bits == (7 as u32)) { return (x << 7) | (x >> 25); }
    if (bits == (9 as u32)) { return (x << 9) | (x >> 23); }
    if (bits == (10 as u32)) { return (x << 10) | (x >> 22); }
    if (bits == (11 as u32)) { return (x << 11) | (x >> 21); }
    if (bits == (12 as u32)) { return (x << 12) | (x >> 20); }
    if (bits == (14 as u32)) { return (x << 14) | (x >> 18); }
    if (bits == (15 as u32)) { return (x << 15) | (x >> 17); }
    if (bits == (16 as u32)) { return (x << 16) | (x >> 16); }
    if (bits == (17 as u32)) { return (x << 17) | (x >> 15); }
    if (bits == (20 as u32)) { return (x << 20) | (x >> 12); }
    if (bits == (21 as u32)) { return (x << 21) | (x >> 11); }
    if (bits == (22 as u32)) { return (x << 22) | (x >> 10); }
    if (bits == (23 as u32)) { return (x << 23) | (x >> 9); }
    return x;
}

fn md5ShiftAt(idx: usize): u32 {
    let phase: usize = idx % (4 as usize);
    if (idx < (16 as usize)) {
        if (phase == (0 as usize)) { return 7 as u32; }
        if (phase == (1 as usize)) { return 12 as u32; }
        if (phase == (2 as usize)) { return 17 as u32; }
        return 22 as u32;
    }
    if (idx < (32 as usize)) {
        if (phase == (0 as usize)) { return 5 as u32; }
        if (phase == (1 as usize)) { return 9 as u32; }
        if (phase == (2 as usize)) { return 14 as u32; }
        return 20 as u32;
    }
    if (idx < (48 as usize)) {
        if (phase == (0 as usize)) { return 4 as u32; }
        if (phase == (1 as usize)) { return 11 as u32; }
        if (phase == (2 as usize)) { return 16 as u32; }
        return 23 as u32;
    }
    if (phase == (0 as usize)) { return 6 as u32; }
    if (phase == (1 as usize)) { return 10 as u32; }
    if (phase == (2 as usize)) { return 15 as u32; }
    return 21 as u32;
}

fn md5ConstAt(idx: usize): u32 {
    let k: [64]u32 = [
        0xd76aa478 as u32, 0xe8c7b756 as u32, 0x242070db as u32, 0xc1bdceee as u32,
        0xf57c0faf as u32, 0x4787c62a as u32, 0xa8304613 as u32, 0xfd469501 as u32,
        0x698098d8 as u32, 0x8b44f7af as u32, 0xffff5bb1 as u32, 0x895cd7be as u32,
        0x6b901122 as u32, 0xfd987193 as u32, 0xa679438e as u32, 0x49b40821 as u32,
        0xf61e2562 as u32, 0xc040b340 as u32, 0x265e5a51 as u32, 0xe9b6c7aa as u32,
        0xd62f105d as u32, 0x02441453 as u32, 0xd8a1e681 as u32, 0xe7d3fbc8 as u32,
        0x21e1cde6 as u32, 0xc33707d6 as u32, 0xf4d50d87 as u32, 0x455a14ed as u32,
        0xa9e3e905 as u32, 0xfcefa3f8 as u32, 0x676f02d9 as u32, 0x8d2a4c8a as u32,
        0xfffa3942 as u32, 0x8771f681 as u32, 0x6d9d6122 as u32, 0xfde5380c as u32,
        0xa4beea44 as u32, 0x4bdecfa9 as u32, 0xf6bb4b60 as u32, 0xbebfbc70 as u32,
        0x289b7ec6 as u32, 0xeaa127fa as u32, 0xd4ef3085 as u32, 0x04881d05 as u32,
        0xd9d4d039 as u32, 0xe6db99e5 as u32, 0x1fa27cf8 as u32, 0xc4ac5665 as u32,
        0xf4292244 as u32, 0x432aff97 as u32, 0xab9423a7 as u32, 0xfc93a039 as u32,
        0x655b59c3 as u32, 0x8f0ccc92 as u32, 0xffeff47d as u32, 0x85845dd1 as u32,
        0x6fa87e4f as u32, 0xfe2ce6e0 as u32, 0xa3014314 as u32, 0x4e0811a1 as u32,
        0xf7537e82 as u32, 0xbd3af235 as u32, 0x2ad7d2bb as u32, 0xeb86d391 as u32,
    ];
    return k[idx];
}

fn md5ReadLe32(b: []u8, off: usize): u32 {
    let b0: u32 = b[off] as u32;
    let b1: u32 = (b[off + (1 as usize)] as u32) << 8;
    let b2: u32 = (b[off + (2 as usize)] as u32) << 16;
    let b3: u32 = (b[off + (3 as usize)] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

fn md5WriteLe32(out: []u8, off: usize, v: u32) {
    out[off] = (v & (0xFF as u32)) as u8;
    out[off + (1 as usize)] = ((v >> 8) & (0xFF as u32)) as u8;
    out[off + (2 as usize)] = ((v >> 16) & (0xFF as u32)) as u8;
    out[off + (3 as usize)] = ((v >> 24) & (0xFF as u32)) as u8;
}

fn md5HexDigit(nibble: u8): u8 {
    if (nibble < (10 as u8)) {
        return ('0' as u8) + nibble;
    }
    return ('a' as u8) + (nibble - (10 as u8));
}

pub struct Md5Hasher {
    a0: u32,
    b0: u32,
    c0: u32,
    d0: u32,
    buf: [64]u8,
    buf_len: usize,
    total_bytes: u64,

    pub static fn new(): Md5Hasher {
        let empty_buf: [64]u8 = undefined;
        let i: usize = 0;
        while (i < MD5_BLOCK_LEN) {
            empty_buf[i] = 0;
            i += 1;
        }
        return Md5Hasher {
            a0: 0x67452301 as u32,
            b0: 0xEFCDAB89 as u32,
            c0: 0x98BADCFE as u32,
            d0: 0x10325476 as u32,
            buf: empty_buf,
            buf_len: 0,
            total_bytes: 0 as u64,
        };
    }

    pub fn reset(*_) {
        _.a0 = 0x67452301 as u32;
        _.b0 = 0xEFCDAB89 as u32;
        _.c0 = 0x98BADCFE as u32;
        _.d0 = 0x10325476 as u32;
        _.buf_len = 0;
        _.total_bytes = 0 as u64;
        let i: usize = 0;
        while (i < MD5_BLOCK_LEN) {
            _.buf[i] = 0;
            i += 1;
        }
    }

    fn compressBuf(*_) {
        let m: [16]u32 = undefined;
        let j: usize = 0;
        while (j < (16 as usize)) {
            m[j] = md5ReadLe32(_.buf[0..64], j * (4 as usize));
            j += 1;
        }

        let a: u32 = _.a0;
        let b: u32 = _.b0;
        let c: u32 = _.c0;
        let d: u32 = _.d0;

        let i: usize = 0;
        while (i < (64 as usize)) {
            let f: u32 = 0 as u32;
            let g: usize = 0;
            if (i < (16 as usize)) {
                let not_b: u32 = b ^ (0xFFFFFFFF as u32);
                f = (b & c) | (not_b & d);
                g = i;
            } else if (i < (32 as usize)) {
                let not_d: u32 = d ^ (0xFFFFFFFF as u32);
                f = (d & b) | (not_d & c);
                g = ((5 as usize) * i + (1 as usize)) % (16 as usize);
            } else if (i < (48 as usize)) {
                f = b ^ c ^ d;
                g = ((3 as usize) * i + (5 as usize)) % (16 as usize);
            } else {
                let not_d: u32 = d ^ (0xFFFFFFFF as u32);
                f = c ^ (b | not_d);
                g = ((7 as usize) * i) % (16 as usize);
            }

            let sum: u32 = md5Add4(a, f, md5ConstAt(i), m[g]);
            let rot: u32 = md5Rotl32(sum, md5ShiftAt(i));
            let next_b: u32 = md5Add32(b, rot);
            a = d;
            d = c;
            c = b;
            b = next_b;
            i += 1;
        }

        _.a0 = md5Add32(_.a0, a);
        _.b0 = md5Add32(_.b0, b);
        _.c0 = md5Add32(_.c0, c);
        _.d0 = md5Add32(_.d0, d);
    }

    pub fn update(*_, data: []u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            _.total_bytes += 1 as u64;
            if (_.buf_len == MD5_BLOCK_LEN) {
                _.compressBuf();
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
            _.total_bytes += 1 as u64;
            if (_.buf_len == MD5_BLOCK_LEN) {
                _.compressBuf();
                _.buf_len = 0;
            }
            i += 1;
        }
    }

    pub fn finalize(*_, out16: []u8) {
        if (out16.len < MD5_DIGEST_LEN) {
            return;
        }

        let total_bits: u64 = _.total_bytes * (8 as u64);

        _.buf[_.buf_len] = 0x80 as u8;
        _.buf_len += 1;

        if (_.buf_len > (56 as usize)) {
            while (_.buf_len < MD5_BLOCK_LEN) {
                _.buf[_.buf_len] = 0;
                _.buf_len += 1;
            }
            _.compressBuf();
            _.buf_len = 0;
        }

        while (_.buf_len < (56 as usize)) {
            _.buf[_.buf_len] = 0;
            _.buf_len += 1;
        }

        _.buf[56] = (total_bits & (0xFF as u64)) as u8;
        _.buf[57] = ((total_bits >> 8) & (0xFF as u64)) as u8;
        _.buf[58] = ((total_bits >> 16) & (0xFF as u64)) as u8;
        _.buf[59] = ((total_bits >> 24) & (0xFF as u64)) as u8;
        _.buf[60] = ((total_bits >> 32) & (0xFF as u64)) as u8;
        _.buf[61] = ((total_bits >> 40) & (0xFF as u64)) as u8;
        _.buf[62] = ((total_bits >> 48) & (0xFF as u64)) as u8;
        _.buf[63] = ((total_bits >> 56) & (0xFF as u64)) as u8;
        _.buf_len = MD5_BLOCK_LEN;
        _.compressBuf();
        _.buf_len = 0;

        md5WriteLe32(out16, 0 as usize, _.a0);
        md5WriteLe32(out16, 4 as usize, _.b0);
        md5WriteLe32(out16, 8 as usize, _.c0);
        md5WriteLe32(out16, 12 as usize, _.d0);
    }

    pub fn finalizeHex(*_, out32: []u8): usize {
        if (out32.len < (32 as usize)) {
            return 0;
        }
        let digest: [16]u8 = undefined;
        _.finalize(digest[0..16]);
        let i: usize = 0;
        while (i < MD5_DIGEST_LEN) {
            let b: u8 = digest[i];
            out32[i * (2 as usize)] = md5HexDigit((b >> 4) & (0x0F as u8));
            out32[i * (2 as usize) + (1 as usize)] = md5HexDigit(b & (0x0F as u8));
            i += 1;
        }
        return 32;
    }
}

pub fn md5Bytes(data: []u8, out16: []u8) {
    let h = Md5Hasher.new();
    h.update(data);
    h.finalize(out16);
}

pub fn md5Hex(s: string, out32: []u8): usize {
    let h = Md5Hasher.new();
    h.updateStr(s);
    return h.finalizeHex(out32);
}
