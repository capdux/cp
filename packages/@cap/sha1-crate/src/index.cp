// @cap/sha1-crate — Pure CP zero-allocation FIPS 180-4 SHA-1 implementation
// Clean-room MIT parity with Rust crates.io `sha1` v0.10.

pub const SHA1_DIGEST_LEN: usize = 20;
pub const SHA1_BLOCK_LEN: usize = 64;

fn sha1Add32(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn sha1Add5(a: u32, b: u32, c: u32, d: u32, e: u32): u32 {
    let sum: u64 = (a as u64) + (b as u64) + (c as u64) + (d as u64) + (e as u64);
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

fn sha1ReadBe32(b: []u8, off: usize): u32 {
    let b0: u32 = (b[off] as u32) << 24;
    let b1: u32 = (b[off + (1 as usize)] as u32) << 16;
    let b2: u32 = (b[off + (2 as usize)] as u32) << 8;
    let b3: u32 = b[off + (3 as usize)] as u32;
    return b0 | b1 | b2 | b3;
}

fn sha1WriteBe32(out: []u8, off: usize, v: u32) {
    out[off] = ((v >> 24) & (0xFF as u32)) as u8;
    out[off + (1 as usize)] = ((v >> 16) & (0xFF as u32)) as u8;
    out[off + (2 as usize)] = ((v >> 8) & (0xFF as u32)) as u8;
    out[off + (3 as usize)] = (v & (0xFF as u32)) as u8;
}

fn sha1HexDigit(nibble: u8): u8 {
    if (nibble < (10 as u8)) {
        return ('0' as u8) + nibble;
    }
    return ('a' as u8) + (nibble - (10 as u8));
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
        let i: usize = 0;
        while (i < SHA1_BLOCK_LEN) {
            empty_buf[i] = 0;
            i += 1;
        }
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
        let i: usize = 0;
        while (i < SHA1_BLOCK_LEN) {
            _.buf[i] = 0;
            i += 1;
        }
    }

    fn compressBuf(*_) {
        let w: [80]u32 = undefined;
        let t: usize = 0;
        while (t < (16 as usize)) {
            w[t] = sha1ReadBe32(_.buf[0..64], t * (4 as usize));
            t += 1;
        }
        while (t < (80 as usize)) {
            let v: u32 = w[t - (3 as usize)] ^ w[t - (8 as usize)] ^ w[t - (14 as usize)] ^ w[t - (16 as usize)];
            w[t] = sha1Rotl1(v);
            t += 1;
        }

        let a: u32 = _.h0;
        let b: u32 = _.h1;
        let c: u32 = _.h2;
        let d: u32 = _.h3;
        let e: u32 = _.h4;

        t = 0;
        while (t < (80 as usize)) {
            let f: u32 = 0 as u32;
            let k: u32 = 0 as u32;
            if (t < (20 as usize)) {
                let not_b: u32 = b ^ (0xFFFFFFFF as u32);
                f = (b & c) | (not_b & d);
                k = 0x5A827999 as u32;
            } else if (t < (40 as usize)) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1 as u32;
            } else if (t < (60 as usize)) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC as u32;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6 as u32;
            }

            let temp: u32 = sha1Add5(sha1Rotl5(a), f, e, k, w[t]);
            e = d;
            d = c;
            c = sha1Rotl30(b);
            b = a;
            a = temp;
            t += 1;
        }

        _.h0 = sha1Add32(_.h0, a);
        _.h1 = sha1Add32(_.h1, b);
        _.h2 = sha1Add32(_.h2, c);
        _.h3 = sha1Add32(_.h3, d);
        _.h4 = sha1Add32(_.h4, e);
    }

    pub fn update(*_, data: []u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            _.total_bytes += 1 as u64;
            if (_.buf_len == SHA1_BLOCK_LEN) {
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
            if (_.buf_len == SHA1_BLOCK_LEN) {
                _.compressBuf();
                _.buf_len = 0;
            }
            i += 1;
        }
    }

    pub fn finalize(*_, out20: []u8) {
        if (out20.len < SHA1_DIGEST_LEN) {
            return;
        }

        let total_bits: u64 = _.total_bytes * (8 as u64);

        _.buf[_.buf_len] = 0x80 as u8;
        _.buf_len += 1;

        if (_.buf_len > (56 as usize)) {
            while (_.buf_len < SHA1_BLOCK_LEN) {
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

        _.buf[56] = ((total_bits >> 56) & (0xFF as u64)) as u8;
        _.buf[57] = ((total_bits >> 48) & (0xFF as u64)) as u8;
        _.buf[58] = ((total_bits >> 40) & (0xFF as u64)) as u8;
        _.buf[59] = ((total_bits >> 32) & (0xFF as u64)) as u8;
        _.buf[60] = ((total_bits >> 24) & (0xFF as u64)) as u8;
        _.buf[61] = ((total_bits >> 16) & (0xFF as u64)) as u8;
        _.buf[62] = ((total_bits >> 8) & (0xFF as u64)) as u8;
        _.buf[63] = (total_bits & (0xFF as u64)) as u8;
        _.buf_len = SHA1_BLOCK_LEN;
        _.compressBuf();
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
        let i: usize = 0;
        while (i < SHA1_DIGEST_LEN) {
            let b: u8 = digest[i];
            out40[i * (2 as usize)] = sha1HexDigit((b >> 4) & (0x0F as u8));
            out40[i * (2 as usize) + (1 as usize)] = sha1HexDigit(b & (0x0F as u8));
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
