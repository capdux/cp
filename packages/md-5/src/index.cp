// @cap/md5-crate — Pure CP zero-allocation RFC 1321 MD5 implementation
// Clean-room MIT parity with Rust crates.io `md-5` / `md5`.
// Accelerated with comptime constant tables, hardware wrapping arithmetic,
// zero-copy 64-byte block compression, bulk @memcpy/@memset, and 16-lane @Vector(16, u8) SIMD hex encoding.

type Md5Vec16 = @Vector(16, u8);

pub const MD5_DIGEST_LEN: usize = 16;
pub const MD5_BLOCK_LEN: usize = 64;

const MD5_K: [64]u32 = [
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

const MD5_SHIFTS: [64]u5 = [
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
];

fn md5Rotl32(x: u32, bits: u5): u32 {
    const rshift: u5 = @intCast((32 as u32) - (bits as u32));
    return (x << bits) | (x >> rshift);
}

fn md5ReadLe32(b: []const u8, off: usize): u32 {
    return (b[off] as u32)
        | ((b[off + 1] as u32) << 8)
        | ((b[off + 2] as u32) << 16)
        | ((b[off + 3] as u32) << 24);
}

fn md5WriteLe32(out: []u8, off: usize, v: u32) {
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
    out[off + 2] = ((v >> 16) & 0xFF) as u8;
    out[off + 3] = ((v >> 24) & 0xFF) as u8;
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
    }

    fn compressBlock(*_, blk: []const u8, off: usize) {
        let m: [16]u32 = undefined;
        let j: usize = 0;
        while (j < 16) {
            m[j] = md5ReadLe32(blk, off + j * 4);
            j += 1;
        }

        let a: u32 = _.a0;
        let b: u32 = _.b0;
        let c: u32 = _.c0;
        let d: u32 = _.d0;

        let i: usize = 0;
        while (i < 16) {
            const f: u32 = (b & c) | ((~b) & d);
            const sum: u32 = a +% f +% MD5_K[i] +% m[i];
            const next_b: u32 = b +% md5Rotl32(sum, MD5_SHIFTS[i]);
            a = d;
            d = c;
            c = b;
            b = next_b;
            i += 1;
        }
        while (i < 32) {
            const f: u32 = (d & b) | ((~d) & c);
            const g: usize = (5 * i + 1) & 15;
            const sum: u32 = a +% f +% MD5_K[i] +% m[g];
            const next_b: u32 = b +% md5Rotl32(sum, MD5_SHIFTS[i]);
            a = d;
            d = c;
            c = b;
            b = next_b;
            i += 1;
        }
        while (i < 48) {
            const f: u32 = b ^ c ^ d;
            const g: usize = (3 * i + 5) & 15;
            const sum: u32 = a +% f +% MD5_K[i] +% m[g];
            const next_b: u32 = b +% md5Rotl32(sum, MD5_SHIFTS[i]);
            a = d;
            d = c;
            c = b;
            b = next_b;
            i += 1;
        }
        while (i < 64) {
            const f: u32 = c ^ (b | (~d));
            const g: usize = (7 * i) & 15;
            const sum: u32 = a +% f +% MD5_K[i] +% m[g];
            const next_b: u32 = b +% md5Rotl32(sum, MD5_SHIFTS[i]);
            a = d;
            d = c;
            c = b;
            b = next_b;
            i += 1;
        }

        _.a0 = _.a0 +% a;
        _.b0 = _.b0 +% b;
        _.c0 = _.c0 +% c;
        _.d0 = _.d0 +% d;
    }

    fn updateSlice(*_, data: []const u8) {
        if (data.len == 0) {
            return;
        }
        _.total_bytes += data.len as u64;
        let pos: usize = 0;

        if (_.buf_len > 0) {
            const need = MD5_BLOCK_LEN - _.buf_len;
            const take = if (data.len < need) data.len else need;
            @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
            _.buf_len += take;
            pos += take;
            if (_.buf_len == MD5_BLOCK_LEN) {
                let tmp: [64]u8 = undefined;
                @memcpy(tmp[0..64], _.buf[0..64]);
                _.compressBlock(tmp[0..64], 0);
                _.buf_len = 0;
            }
        }

        while (pos + MD5_BLOCK_LEN <= data.len) {
            _.compressBlock(data, pos);
            pos += MD5_BLOCK_LEN;
        }

        const rem = data.len - pos;
        if (rem > 0) {
            @memcpy(_.buf[0..rem], data[pos..data.len]);
            _.buf_len = rem;
        }
    }

    pub fn update(*_, data: []u8) {
        _.updateSlice(data);
    }

    pub fn updateStr(*_, s: string) {
        _.updateSlice(s);
    }

    pub fn finalize(*_, out16: []u8) {
        if (out16.len < MD5_DIGEST_LEN) {
            return;
        }

        const total_bits: u64 = _.total_bytes << 3;

        _.buf[_.buf_len] = 0x80 as u8;
        _.buf_len += 1;

        if (_.buf_len > 56) {
            if (_.buf_len < MD5_BLOCK_LEN) {
                @memset(_.buf[_.buf_len..MD5_BLOCK_LEN], 0);
            }
            let tmp: [64]u8 = undefined;
            @memcpy(tmp[0..64], _.buf[0..64]);
            _.compressBlock(tmp[0..64], 0);
            _.buf_len = 0;
        }

        if (_.buf_len < 56) {
            @memset(_.buf[_.buf_len..56], 0);
        }

        _.buf[56] = (total_bits & 0xFF) as u8;
        _.buf[57] = ((total_bits >> 8) & 0xFF) as u8;
        _.buf[58] = ((total_bits >> 16) & 0xFF) as u8;
        _.buf[59] = ((total_bits >> 24) & 0xFF) as u8;
        _.buf[60] = ((total_bits >> 32) & 0xFF) as u8;
        _.buf[61] = ((total_bits >> 40) & 0xFF) as u8;
        _.buf[62] = ((total_bits >> 48) & 0xFF) as u8;
        _.buf[63] = ((total_bits >> 56) & 0xFF) as u8;
        let final_blk: [64]u8 = undefined;
        @memcpy(final_blk[0..64], _.buf[0..64]);
        _.compressBlock(final_blk[0..64], 0);
        _.buf_len = 0;

        md5WriteLe32(out16, 0, _.a0);
        md5WriteLe32(out16, 4, _.b0);
        md5WriteLe32(out16, 8, _.c0);
        md5WriteLe32(out16, 12, _.d0);
    }

    pub fn finalizeHex(*_, out32: []u8): usize {
        if (out32.len < 32) {
            return 0;
        }
        let digest: [16]u8 = undefined;
        _.finalize(digest[0..16]);

        // 16-lane SIMD nibble-to-ASCII-hex conversion
        const v: Md5Vec16 = digest;
        const v_zero: Md5Vec16 = @splat(48 as u8);
        const v_ten: Md5Vec16 = @splat(10 as u8);
        const v_alpha: Md5Vec16 = @splat(39 as u8);
        const v_nul: Md5Vec16 = @splat(0 as u8);
        const v_mask: Md5Vec16 = @splat(0x0F as u8);
        const v_sh4: @Vector(16, u3) = @splat(4 as u3);

        const hi_n: Md5Vec16 = v >> v_sh4;
        const lo_n: Md5Vec16 = v & v_mask;
        const hi_c: [16]u8 = hi_n + v_zero + @select(u8, hi_n >= v_ten, v_alpha, v_nul);
        const lo_c: [16]u8 = lo_n + v_zero + @select(u8, lo_n >= v_ten, v_alpha, v_nul);

        let i: usize = 0;
        while (i < 16) {
            out32[i * 2] = hi_c[i];
            out32[i * 2 + 1] = lo_c[i];
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
