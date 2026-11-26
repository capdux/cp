// @cap/murmur3 — Zero-heap MurmurHash3 (32-bit x86 & 128-bit x64)
// Clean-room MIT implementation in pure CP (replaces Rust `murmur3` & npm `murmurhash3js`).

fn wrapAdd32(a: u32, b: u32) -> u32 {
    const s: u64 = (a as u64) + (b as u64);
    return (s & 0xFFFFFFFF) as u32;
}

fn wrapMul32(a: u32, b: u32) -> u32 {
    const p: u64 = (a as u64) * (b as u64);
    return (p & 0xFFFFFFFF) as u32;
}

fn rotl32(x: u32, r: u32) -> u32 {
    return (x << r) | (x >> (32 - r));
}

fn wrapAdd64(a: u64, b: u64) -> u64 {
    const lo: u64 = (a & 0xFFFFFFFF) + (b & 0xFFFFFFFF);
    const hi: u64 = (a >> 32) + (b >> 32) + (lo >> 32);
    return ((hi & 0xFFFFFFFF) << 32) | (lo & 0xFFFFFFFF);
}

fn wrapMul64(a: u64, b: u64) -> u64 {
    const a0: u64 = a & 0xFFFFFFFF;
    const a1: u64 = a >> 32;
    const b0: u64 = b & 0xFFFFFFFF;
    const b1: u64 = b >> 32;
    const p00: u64 = a0 * b0;
    const p01: u64 = a0 * b1;
    const p10: u64 = a1 * b0;
    const hi: u64 = ((p00 >> 32) + (p01 & 0xFFFFFFFF) + (p10 & 0xFFFFFFFF)) & 0xFFFFFFFF;
    return (hi << 32) | (p00 & 0xFFFFFFFF);
}

fn rotl64(x: u64, r: u32) -> u64 {
    return (x << r) | (x >> (64 - r));
}

fn fmix32(h_in: u32) -> u32 {
    let h: u32 = h_in;
    h = h ^ (h >> 16);
    h = wrapMul32(h, 0x85ebca6b as u32);
    h = h ^ (h >> 13);
    h = wrapMul32(h, 0xc2b2ae35 as u32);
    h = h ^ (h >> 16);
    return h;
}

fn fmix64(k_in: u64) -> u64 {
    let k: u64 = k_in;
    k = k ^ (k >> 33);
    k = wrapMul64(k, 0xff51afd7ed558ccd as u64);
    k = k ^ (k >> 33);
    k = wrapMul64(k, 0xc4ceb9fe1a85ec53 as u64);
    k = k ^ (k >> 33);
    return k;
}

/// Compute MurmurHash3 32-bit (x86) hash of `data` with `seed`.
pub fn murmur3_32(data: string, seed: u32) -> u32 {
    const c1: u32 = 0xcc9e2d51 as u32;
    const c2: u32 = 0x1b873593 as u32;

    const nblocks: usize = data.len / 4;
    let h1: u32 = seed;

    let i: usize = 0;
    while (i < nblocks) {
        const off: usize = i * 4;
        let k1: u32 = (data[off] as u32) |
            ((data[off + 1] as u32) << 8) |
            ((data[off + 2] as u32) << 16) |
            ((data[off + 3] as u32) << 24);
        k1 = wrapMul32(k1, c1);
        k1 = rotl32(k1, 15);
        k1 = wrapMul32(k1, c2);

        h1 = h1 ^ k1;
        h1 = rotl32(h1, 13);
        h1 = wrapAdd32(wrapMul32(h1, 5), 0xe6546b64 as u32);
        i += 1;
    }

    const tailOff: usize = nblocks * 4;
    const rem: usize = data.len - tailOff;
    let k1: u32 = 0;
    if (rem >= 3) { k1 = k1 ^ ((data[tailOff + 2] as u32) << 16); }
    if (rem >= 2) { k1 = k1 ^ ((data[tailOff + 1] as u32) << 8); }
    if (rem >= 1) {
        k1 = k1 ^ (data[tailOff] as u32);
        k1 = wrapMul32(k1, c1);
        k1 = rotl32(k1, 15);
        k1 = wrapMul32(k1, c2);
        h1 = h1 ^ k1;
    }

    h1 = h1 ^ (data.len as u32);
    return fmix32(h1);
}

pub struct Hash128 {
    pub lo: u64,
    pub hi: u64,
}

/// Compute MurmurHash3 128-bit (x64) hash of `data` with `seed`.
pub fn murmur3_128(data: string, seed: u64) -> Hash128 {
    const c1: u64 = 0x87c37b91114253d5 as u64;
    const c2: u64 = 0x4cf5ad432745937f as u64;

    const nblocks: usize = data.len / 16;
    let h1: u64 = seed;
    let h2: u64 = seed;

    let i: usize = 0;
    while (i < nblocks) {
        const off: usize = i * 16;
        let k1: u64 = (data[off] as u64) |
            ((data[off + 1] as u64) << 8) |
            ((data[off + 2] as u64) << 16) |
            ((data[off + 3] as u64) << 24) |
            ((data[off + 4] as u64) << 32) |
            ((data[off + 5] as u64) << 40) |
            ((data[off + 6] as u64) << 48) |
            ((data[off + 7] as u64) << 56);
        let k2: u64 = (data[off + 8] as u64) |
            ((data[off + 9] as u64) << 8) |
            ((data[off + 10] as u64) << 16) |
            ((data[off + 11] as u64) << 24) |
            ((data[off + 12] as u64) << 32) |
            ((data[off + 13] as u64) << 40) |
            ((data[off + 14] as u64) << 48) |
            ((data[off + 15] as u64) << 56);

        k1 = wrapMul64(k1, c1);
        k1 = rotl64(k1, 31);
        k1 = wrapMul64(k1, c2);
        h1 = h1 ^ k1;
        h1 = rotl64(h1, 27);
        h1 = wrapAdd64(h1, h2);
        h1 = wrapAdd64(wrapMul64(h1, 5), 0x52dce729 as u64);

        k2 = wrapMul64(k2, c2);
        k2 = rotl64(k2, 33);
        k2 = wrapMul64(k2, c1);
        h2 = h2 ^ k2;
        h2 = rotl64(h2, 31);
        h2 = wrapAdd64(h2, h1);
        h2 = wrapAdd64(wrapMul64(h2, 5), 0x38495ab5 as u64);
        i += 1;
    }

    const tailOff: usize = nblocks * 16;
    const rem: usize = data.len - tailOff;
    let k1: u64 = 0;
    let k2: u64 = 0;

    if (rem >= 15) { k2 = k2 ^ ((data[tailOff + 14] as u64) << 48); }
    if (rem >= 14) { k2 = k2 ^ ((data[tailOff + 13] as u64) << 40); }
    if (rem >= 13) { k2 = k2 ^ ((data[tailOff + 12] as u64) << 32); }
    if (rem >= 12) { k2 = k2 ^ ((data[tailOff + 11] as u64) << 24); }
    if (rem >= 11) { k2 = k2 ^ ((data[tailOff + 10] as u64) << 16); }
    if (rem >= 10) { k2 = k2 ^ ((data[tailOff + 9] as u64) << 8); }
    if (rem >= 9) {
        k2 = k2 ^ (data[tailOff + 8] as u64);
        k2 = wrapMul64(k2, c2);
        k2 = rotl64(k2, 33);
        k2 = wrapMul64(k2, c1);
        h2 = h2 ^ k2;
    }

    if (rem >= 8) { k1 = k1 ^ ((data[tailOff + 7] as u64) << 56); }
    if (rem >= 7) { k1 = k1 ^ ((data[tailOff + 6] as u64) << 48); }
    if (rem >= 6) { k1 = k1 ^ ((data[tailOff + 5] as u64) << 40); }
    if (rem >= 5) { k1 = k1 ^ ((data[tailOff + 4] as u64) << 32); }
    if (rem >= 4) { k1 = k1 ^ ((data[tailOff + 3] as u64) << 24); }
    if (rem >= 3) { k1 = k1 ^ ((data[tailOff + 2] as u64) << 16); }
    if (rem >= 2) { k1 = k1 ^ ((data[tailOff + 1] as u64) << 8); }
    if (rem >= 1) {
        k1 = k1 ^ (data[tailOff] as u64);
        k1 = wrapMul64(k1, c1);
        k1 = rotl64(k1, 31);
        k1 = wrapMul64(k1, c2);
        h1 = h1 ^ k1;
    }

    h1 = h1 ^ (data.len as u64);
    h2 = h2 ^ (data.len as u64);
    h1 = wrapAdd64(h1, h2);
    h2 = wrapAdd64(h2, h1);
    h1 = fmix64(h1);
    h2 = fmix64(h2);
    h1 = wrapAdd64(h1, h2);
    h2 = wrapAdd64(h2, h1);

    return Hash128 { lo: h1, hi: h2 };
}
