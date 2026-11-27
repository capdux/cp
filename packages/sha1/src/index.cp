// @cap/sha1-crate — Pure CP zero-allocation FIPS 180-4 SHA-1 implementation
// Clean-room MIT parity with Rust crates.io `sha1` v0.10.
// Accelerated with 16-lane @Vector(16, u8) SIMD hex encoding, comptime 256-entry hex LUTs, and zero-copy bulk @memcpy/@memset.

pub const SHA1_DIGEST_LEN: usize = 20;
pub const SHA1_BLOCK_LEN: usize = 64;

type Sha1Vec16 = @Vector(16, u8);

fn sha1BuildHexHiLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    const hex = "0123456789abcdef";
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        lut[i] = hex[(i >> 4) & 0x0F];
        i += 1;
    }
    return lut;
}

fn sha1BuildHexLoLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    const hex = "0123456789abcdef";
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        lut[i] = hex[i & 0x0F];
        i += 1;
    }
    return lut;
}

const SHA1_HEX_HI: [256]u8 = sha1BuildHexHiLut();
const SHA1_HEX_LO: [256]u8 = sha1BuildHexLoLut();

fn sha1Add32(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn sha1Add5(a: u32, b: u32, c: u32, d: u32, e: u32): u32 {
    const sum: u64 = (a as u64) + (b as u64) + (c as u64) + (d as u64) + (e as u64);
    return (sum & (0xFFFFFFFF as u64)) as u32;
}

fn sha1Rotl1(x: u32): u32 {
    return (x << 1) | (x >> 31);
}

fn sha1Rotl5(x: u32): u32 {
    return (x << 5) | (x >> 27);
}

fn sha1Rotl30(x: u32): u32 {
    return (x << 30) | (x >> 2);
}

fn sha1ReadBe32(b: []const u8, off: usize): u32 {
    const b0: u32 = (b[off] as u32) << 24;
    const b1: u32 = (b[off + (1 as usize)] as u32) << 16;
    const b2: u32 = (b[off + (2 as usize)] as u32) << 8;
    const b3: u32 = b[off + (3 as usize)] as u32;
    return b0 | b1 | b2 | b3;
}

fn sha1WriteBe32(out: []u8, off: usize, v: u32) {
    out[off] = ((v >> 24) & (0xFF as u32)) as u8;
    out[off + (1 as usize)] = ((v >> 16) & (0xFF as u32)) as u8;
    out[off + (2 as usize)] = ((v >> 8) & (0xFF as u32)) as u8;
    out[off + (3 as usize)] = (v & (0xFF as u32)) as u8;
}

// 16-lane SIMD branchless hex encoding for 16 bytes -> 32 ASCII hex bytes
fn sha1EncodeHex16Simd(src: []const u8, dst: []u8) {
    const v: Sha1Vec16 = src[0..16].*;
    const v_mask: Sha1Vec16 = @splat(0x0F as u8);
    const v_ten: Sha1Vec16 = @splat(10 as u8);
    const v_48: Sha1Vec16 = @splat(48 as u8);
    const v_87: Sha1Vec16 = @splat(87 as u8);
    const v_shift: @Vector(16, u3) = @splat(4 as u3);

    const hi_nib: Sha1Vec16 = v >> v_shift;
    const lo_nib: Sha1Vec16 = v & v_mask;
    const hi_ascii: [16]u8 = @select(u8, hi_nib < v_ten, hi_nib + v_48, hi_nib + v_87);
    const lo_ascii: [16]u8 = @select(u8, lo_nib < v_ten, lo_nib + v_48, lo_nib + v_87);

    let i: usize = 0;
    while (i < 16) {
        dst[i * 2] = hi_ascii[i];
        dst[i * 2 + 1] = lo_ascii[i];
        i += 1;
    }
}

pub struct Sha1Hasher {
    h0: u32,
    h1: u32,
    h2: u32,
    h3: u32,
    h4: u32,
    buf: [64]u8,
    buf_len: usize,
    total_bytes: u64,

    pub static fn new(): Sha1Hasher {
        let empty_buf: [64]u8 = undefined;
        @memset(empty_buf[0..SHA1_BLOCK_LEN], 0);
        return Sha1Hasher {
            h0: 0x67452301 as u32,
            h1: 0xEFCDAB89 as u32,
            h2: 0x98BADCFE as u32,
            h3: 0x10325476 as u32,
            h4: 0xC3D2E1F0 as u32,
            buf: empty_buf,
            buf_len: 0,
            total_bytes: 0 as u64,
        };
    }

    pub fn reset(*_) {
        _.h0 = 0x67452301 as u32;
        _.h1 = 0xEFCDAB89 as u32;
        _.h2 = 0x98BADCFE as u32;
        _.h3 = 0x10325476 as u32;
        _.h4 = 0xC3D2E1F0 as u32;
        _.buf_len = 0;
        _.total_bytes = 0 as u64;
        @memset(_.buf[0..SHA1_BLOCK_LEN], 0);
    }

    fn compressBlock(*_, block: []const u8) {
        let w: [80]u32 = undefined;
        let t: usize = 0;
        while (t < (16 as usize)) {
            w[t] = sha1ReadBe32(block, t * (4 as usize));
            t += 1;
        }
        while (t < (80 as usize)) {
            const v: u32 = w[t - (3 as usize)] ^ w[t - (8 as usize)] ^ w[t - (14 as usize)] ^ w[t - (16 as usize)];
            w[t] = sha1Rotl1(v);
            t += 1;
        }

        let a: u32 = _.h0;
        let b: u32 = _.h1;
        let c: u32 = _.h2;
        let d: u32 = _.h3;
        let e: u32 = _.h4;

        t = 0;
        while (t < (20 as usize)) {
            const not_b: u32 = b ^ (0xFFFFFFFF as u32);
            const f: u32 = (b & c) | (not_b & d);
            const temp: u32 = sha1Add5(sha1Rotl5(a), f, e, 0x5A827999 as u32, w[t]);
            e = d; d = c; c = sha1Rotl30(b); b = a; a = temp;
            t += 1;
        }
        while (t < (40 as usize)) {
            const f: u32 = b ^ c ^ d;
            const temp: u32 = sha1Add5(sha1Rotl5(a), f, e, 0x6ED9EBA1 as u32, w[t]);
            e = d; d = c; c = sha1Rotl30(b); b = a; a = temp;
            t += 1;
        }
        while (t < (60 as usize)) {
            const f: u32 = (b & c) | (b & d) | (c & d);
            const temp: u32 = sha1Add5(sha1Rotl5(a), f, e, 0x8F1BBCDC as u32, w[t]);
            e = d; d = c; c = sha1Rotl30(b); b = a; a = temp;
            t += 1;
        }
        while (t < (80 as usize)) {
            const f: u32 = b ^ c ^ d;
            const temp: u32 = sha1Add5(sha1Rotl5(a), f, e, 0xCA62C1D6 as u32, w[t]);
            e = d; d = c; c = sha1Rotl30(b); b = a; a = temp;
            t += 1;
        }

        _.h0 = sha1Add32(_.h0, a);
        _.h1 = sha1Add32(_.h1, b);
        _.h2 = sha1Add32(_.h2, c);
        _.h3 = sha1Add32(_.h3, d);
        _.h4 = sha1Add32(_.h4, e);
    }

    fn updateSlice(*_, data: []const u8) {
        _.total_bytes += data.len as u64;
        let i: usize = 0;
        if (_.buf_len > 0) {
            let take = SHA1_BLOCK_LEN - _.buf_len;
            if (take > data.len) {
                take = data.len;
            }
            if (take > 0) {
                @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
                _.buf_len += take;
                i += take;
            }
            if (_.buf_len == SHA1_BLOCK_LEN) {
                _.compressBlock(_.buf[0..64]);
                _.buf_len = 0;
            }
        }
        while (i + SHA1_BLOCK_LEN <= data.len) {
            _.compressBlock(data[i .. i + SHA1_BLOCK_LEN]);
            i += SHA1_BLOCK_LEN;
        }
        const rem = data.len - i;
        if (rem > 0) {
            @memcpy(_.buf[_.buf_len .. _.buf_len + rem], data[i..data.len]);
            _.buf_len += rem;
        }
    }

    pub fn update(*_, data: []u8) {
        _.updateSlice(data);
    }

    pub fn updateStr(*_, s: string) {
        _.updateSlice(s);
    }

    pub fn finalize(*_, out20: []u8) {
        if (out20.len < SHA1_DIGEST_LEN) {
            return;
        }

        const total_bits: u64 = _.total_bytes * (8 as u64);

        _.buf[_.buf_len] = 0x80 as u8;
        _.buf_len += 1;

        if (_.buf_len > (56 as usize)) {
            @memset(_.buf[_.buf_len..SHA1_BLOCK_LEN], 0);
            _.compressBlock(_.buf[0..64]);
            _.buf_len = 0;
        }

        @memset(_.buf[_.buf_len..56], 0);

        _.buf[56] = ((total_bits >> 56) & (0xFF as u64)) as u8;
        _.buf[57] = ((total_bits >> 48) & (0xFF as u64)) as u8;
        _.buf[58] = ((total_bits >> 40) & (0xFF as u64)) as u8;
        _.buf[59] = ((total_bits >> 32) & (0xFF as u64)) as u8;
        _.buf[60] = ((total_bits >> 24) & (0xFF as u64)) as u8;
        _.buf[61] = ((total_bits >> 16) & (0xFF as u64)) as u8;
        _.buf[62] = ((total_bits >> 8) & (0xFF as u64)) as u8;
        _.buf[63] = (total_bits & (0xFF as u64)) as u8;
        _.buf_len = SHA1_BLOCK_LEN;
        _.compressBlock(_.buf[0..64]);
        _.buf_len = 0;

        sha1WriteBe32(out20, 0 as usize, _.h0);
        sha1WriteBe32(out20, 4 as usize, _.h1);
        sha1WriteBe32(out20, 8 as usize, _.h2);
        sha1WriteBe32(out20, 12 as usize, _.h3);
        sha1WriteBe32(out20, 16 as usize, _.h4);
    }

    pub fn finalizeHex(*_, out40: []u8): usize {
        if (out40.len < (40 as usize)) {
            return 0;
        }
        let digest: [20]u8 = undefined;
        _.finalize(digest[0..20]);
        sha1EncodeHex16Simd(digest[0..16], out40[0..32]);
        let i: usize = 16;
        while (i < SHA1_DIGEST_LEN) {
            const b = digest[i] as usize;
            out40[i * (2 as usize)] = SHA1_HEX_HI[b];
            out40[i * (2 as usize) + (1 as usize)] = SHA1_HEX_LO[b];
            i += 1;
        }
        return 40;
    }
}

pub fn sha1Bytes(data: []u8, out20: []u8) {
    let h = Sha1Hasher.new();
    h.update(data);
    h.finalize(out20);
}

pub fn sha1Hex(s: string, out40: []u8): usize {
    let h = Sha1Hasher.new();
    h.updateStr(s);
    return h.finalizeHex(out40);
}
