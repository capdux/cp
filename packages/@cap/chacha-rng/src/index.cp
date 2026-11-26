// packages/@cap/chacha-rng/src/index.cp
// Zero-allocation RFC 8439 ChaCha8 / ChaCha12 / ChaCha20 core block & CSPRNG.
// MIT clean-room pure CP implementation.

fn crAdd32(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn crRotl32(x: u32, bits: u32): u32 {
    const s1 = bits as u5;
    const s2 = ((32 as u32) - bits) as u5;
    return (x << s1) | (x >> s2);
}

fn crReadLe32(data: []const u8, offset: usize): u32 {
    let v: u32 = data[offset + 0] as u32;
    v = v | ((data[offset + 1] as u32) << (8 as u5));
    v = v | ((data[offset + 2] as u32) << (16 as u5));
    v = v | ((data[offset + 3] as u32) << (24 as u5));
    return v;
}

fn crWriteLe32(out: []u8, offset: usize, v: u32) {
    out[offset + 0] = (v & (0xFF as u32)) as u8;
    out[offset + 1] = ((v >> (8 as u5)) & (0xFF as u32)) as u8;
    out[offset + 2] = ((v >> (16 as u5)) & (0xFF as u32)) as u8;
    out[offset + 3] = ((v >> (24 as u5)) & (0xFF as u32)) as u8;
}

fn crQuarterRound(st: []u32, a: usize, b: usize, c: usize, d: usize) {
    let va = st[a];
    let vb = st[b];
    let vc = st[c];
    let vd = st[d];

    va = crAdd32(va, vb);
    vd = crRotl32(vd ^ va, 16 as u32);

    vc = crAdd32(vc, vd);
    vb = crRotl32(vb ^ vc, 12 as u32);

    va = crAdd32(va, vb);
    vd = crRotl32(vd ^ va, 8 as u32);

    vc = crAdd32(vc, vd);
    vb = crRotl32(vb ^ vc, 7 as u32);

    st[a] = va;
    st[b] = vb;
    st[c] = vc;
    st[d] = vd;
}

/// RFC 8439 ChaCha core block function supporting ChaCha8 (`rounds = 8`),
/// ChaCha12 (`rounds = 12`), and ChaCha20 (`rounds = 20`).
/// Writes 64 little-endian keystream bytes into `out[0..64]`. No-op if `out.len < 64`.
pub fn chachaBlock(key: [8]u32, counter: u32, nonce: [3]u32, rounds: u32, out: []u8) {
    if (out.len < 64) {
        return;
    }

    let initial: [16]u32 = undefined;
    // RFC 8439 constants: "expand 32-byte k"
    initial[0] = 0x61707865 as u32;
    initial[1] = 0x3320646E as u32;
    initial[2] = 0x79622D32 as u32;
    initial[3] = 0x6B206574 as u32;

    let ki: usize = 0;
    while (ki < 8) {
        initial[4 + ki] = key[ki];
        ki += 1;
    }
    initial[12] = counter;
    initial[13] = nonce[0];
    initial[14] = nonce[1];
    initial[15] = nonce[2];

    let work: [16]u32 = undefined;
    let wi: usize = 0;
    while (wi < 16) {
        work[wi] = initial[wi];
        wi += 1;
    }

    const doubleRounds = rounds / (2 as u32);
    let r: u32 = 0 as u32;
    while (r < doubleRounds) {
        // Column rounds
        crQuarterRound(work[0..16], 0, 4, 8, 12);
        crQuarterRound(work[0..16], 1, 5, 9, 13);
        crQuarterRound(work[0..16], 2, 6, 10, 14);
        crQuarterRound(work[0..16], 3, 7, 11, 15);
        // Diagonal rounds
        crQuarterRound(work[0..16], 0, 5, 10, 15);
        crQuarterRound(work[0..16], 1, 6, 11, 12);
        crQuarterRound(work[0..16], 2, 7, 8, 13);
        crQuarterRound(work[0..16], 3, 4, 9, 14);
        r += 1 as u32;
    }

    let oi: usize = 0;
    while (oi < 16) {
        const word = crAdd32(work[oi], initial[oi]);
        crWriteLe32(out, oi * 4, word);
        oi += 1;
    }
}

/// Zero-allocation ChaCha20 CSPRNG with seekable 32-bit block counter.
pub struct ChaCha20Rng {
    pub key: [8]u32,
    pub nonce: [3]u32,
    pub counter: u32,
    pub buf: [64]u8,
    pub pos: usize,

    /// Initialize a ChaCha20 CSPRNG from a 32-byte (256-bit) seed.
    pub static fn fromSeed(seed: [32]u8): ChaCha20Rng {
        let k: [8]u32 = undefined;
        let i: usize = 0;
        while (i < 8) {
            k[i] = crReadLe32(seed[0..32], i * 4);
            i += 1;
        }
        let n: [3]u32 = undefined;
        n[0] = 0 as u32;
        n[1] = 0 as u32;
        n[2] = 0 as u32;

        let b: [64]u8 = undefined;
        let bi: usize = 0;
        while (bi < 64) {
            b[bi] = 0 as u8;
            bi += 1;
        }
        return ChaCha20Rng{
            key: k,
            nonce: n,
            counter: 0 as u32,
            buf: b,
            pos: 64,
        };
    }

    /// Initialize a ChaCha20 CSPRNG deterministically from a 64-bit integer seed.
    pub static fn fromU64(seed: u64): ChaCha20Rng {
        let raw: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            raw[i] = 0 as u8;
            i += 1;
        }
        raw[0] = (seed & (0xFF as u64)) as u8;
        raw[1] = ((seed >> (8 as u6)) & (0xFF as u64)) as u8;
        raw[2] = ((seed >> (16 as u6)) & (0xFF as u64)) as u8;
        raw[3] = ((seed >> (24 as u6)) & (0xFF as u64)) as u8;
        raw[4] = ((seed >> (32 as u6)) & (0xFF as u64)) as u8;
        raw[5] = ((seed >> (40 as u6)) & (0xFF as u64)) as u8;
        raw[6] = ((seed >> (48 as u6)) & (0xFF as u64)) as u8;
        raw[7] = ((seed >> (56 as u6)) & (0xFF as u64)) as u8;
        return ChaCha20Rng.fromSeed(raw);
    }

    /// Seek the stream directly to the given 64-byte block counter.
    pub fn setWordPos(*_, block_counter: u32) {
        _.counter = block_counter;
        _.pos = 64;
    }

    /// Fill `out` with cryptographically strong ChaCha20 keystream bytes.
    pub fn fillBytes(*_, out: []u8) {
        let off: usize = 0;
        while (off < out.len) {
            if (_.pos >= 64) {
                chachaBlock(_.key, _.counter, _.nonce, 20 as u32, _.buf[0..64]);
                _.counter = crAdd32(_.counter, 1 as u32);
                _.pos = 0;
            }
            const avail: usize = (64 as usize) - _.pos;
            let take = out.len - off;
            if (take > avail) {
                take = avail;
            }
            let i: usize = 0;
            while (i < take) {
                out[off + i] = _.buf[_.pos + i];
                i += 1;
            }
            _.pos += take;
            off += take;
        }
    }

    /// Generate the next little-endian `u32`.
    pub fn nextU32(*_): u32 {
        let tmp: [4]u8 = undefined;
        _.fillBytes(tmp[0..4]);
        return crReadLe32(tmp[0..4], 0);
    }

    /// Generate the next little-endian `u64`.
    pub fn nextU64(*_): u64 {
        let tmp: [8]u8 = undefined;
        _.fillBytes(tmp[0..8]);
        const lo = crReadLe32(tmp[0..8], 0) as u64;
        const hi = crReadLe32(tmp[0..8], 4) as u64;
        return (hi << (32 as u6)) | lo;
    }

    /// Generate a uniform `f64` in `[0.0, 1.0)` using 53 bits of precision.
    pub fn nextF64(*_): f64 {
        const bits = _.nextU64() >> (11 as u6);
        return (bits as f64) / (9007199254740992.0 as f64);
    }
}

export {
    chachaBlock,
    ChaCha20Rng,
};
