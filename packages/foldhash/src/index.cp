// @cap/foldhash — Zero-allocation 64x64->128-bit folded-multiply hasher (Fast & Quality modes) for CP
// Clean-room MIT implementation inspired by Rust `foldhash` (895M downloads, default hasher in hashbrown 0.15+).

import { readU64LE } from "cap:bin";

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) * (0x100000000 as u64)) | (lo as u64);
}

fn addWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const lo = (a & mask32) + (b & mask32);
    const carry = lo / shift32;
    const hi = ((a / shift32) + (b / shift32) + carry) & mask32;
    return (hi * shift32) | (lo & mask32);
}

// Exact 64x64 -> 128-bit multiplication returning (lo ^ hi) using pure 64-bit arithmetic.
pub fn foldedMultiply(x: u64, y: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;

    const x0 = x & mask32;
    const x1 = x / shift32;
    const y0 = y & mask32;
    const y1 = y / shift32;

    const p00 = x0 * y0;
    const p01 = x0 * y1;
    const p10 = x1 * y0;
    const p11 = x1 * y1;

    const lo_lo = p00 & mask32;
    const mid = (p00 / shift32) + (p01 & mask32) + (p10 & mask32);
    const lo = lo_lo | ((mid & mask32) * shift32);
    const hi = p11 + (p01 / shift32) + (p10 / shift32) + (mid / shift32);

    return lo ^ hi;
}

fn byteShift64(idx: u8) -> u64 {
    let s: u64 = 1;
    let i: u8 = 0;
    while (i < idx) {
        s = s * (256 as u64);
        i += 1 as u8;
    }
    return s;
}

fn initFoldSeed(seed: u64) -> u64 {
    // Odd arbitrary-looking mixing constant derived from golden ratio / pi fractional bits
    const c1 = makeU64(0x2D358DCC as u32, 0xAA6C78A5 as u32);
    const c2 = makeU64(0x8BB84B93 as u32, 0x962EACC9 as u32);
    return foldedMultiply(seed ^ c1, c2) | (1 as u64);
}

fn initAccum(seed: u64) -> u64 {
    const c3 = makeU64(0x4B33A62E as u32, 0xD433D4A3 as u32);
    const c4 = makeU64(0x4D5A2DA5 as u32, 0x1DE1AA47 as u32);
    return foldedMultiply(addWrap64(seed, c3), c4);
}

pub struct FoldHasher {
    pub accum: u64,
    pub sponge: u64,
    pub sponge_len: u8,
    pub fold_seed: u64,
    pub quality: bool,
    pub total_len: u64,

    pub static fn initFast(seed: u64) -> FoldHasher {
        return FoldHasher{
            accum: initAccum(seed),
            sponge: 0 as u64,
            sponge_len: 0 as u8,
            fold_seed: initFoldSeed(seed),
            quality: false,
            total_len: 0 as u64,
        };
    }

    pub static fn initQuality(seed: u64) -> FoldHasher {
        return FoldHasher{
            accum: initAccum(seed),
            sponge: 0 as u64,
            sponge_len: 0 as u8,
            fold_seed: initFoldSeed(seed),
            quality: true,
            total_len: 0 as u64,
        };
    }

    pub fn writeU8(*_, v: u8) {
        _.sponge = _.sponge | ((v as u64) * byteShift64(_.sponge_len));
        _.sponge_len += 1 as u8;
        _.total_len += 1 as u64;
        if (_.sponge_len == (8 as u8)) {
            _.accum = foldedMultiply(_.accum ^ _.sponge, _.fold_seed);
            _.sponge = 0 as u64;
            _.sponge_len = 0 as u8;
        }
    }

    pub fn writeU32(*_, v: u32) {
        _.writeU8((v & (0xFF as u32)) as u8);
        _.writeU8(((v / (256 as u32)) & (0xFF as u32)) as u8);
        _.writeU8(((v / (65536 as u32)) & (0xFF as u32)) as u8);
        _.writeU8(((v / (16777216 as u32)) & (0xFF as u32)) as u8);
    }

    pub fn writeU64(*_, v: u64) {
        if (_.sponge_len == (0 as u8)) {
            _.accum = foldedMultiply(_.accum ^ v, _.fold_seed);
            _.total_len += 8 as u64;
            return;
        }
        let cur = v;
        let i: usize = 0;
        while (i < 8) {
            _.writeU8((cur & (0xFF as u64)) as u8);
            cur = cur / (256 as u64);
            i += 1;
        }
    }

    pub fn writeBytes(*_, data: []const u8) {
        let i: usize = 0;
        // Fill existing partial sponge first so streaming chunks match monolithic hashing
        while (i < data.len && _.sponge_len > (0 as u8)) {
            _.writeU8(data[i]);
            i += 1;
        }
        // Fast 8-byte bulk loop
        while (i + 8 <= data.len) {
            const word = readU64LE(data, i) ?? 0;
            _.accum = foldedMultiply(_.accum ^ word, _.fold_seed);
            _.total_len += 8 as u64;
            i += 8;
        }
        // Remaining 0..7 tail bytes into sponge
        while (i < data.len) {
            _.writeU8(data[i]);
            i += 1;
        }
    }

    pub fn finish() -> u64 {
        let h = _.accum;
        if (_.sponge_len > (0 as u8)) {
            h = foldedMultiply(h ^ _.sponge, _.fold_seed);
        }
        // Mix total byte length to prevent prefix/zero-padding collisions
        const lenMix = makeU64(0xA0761D64 as u32, 0x78BD642F as u32);
        h = foldedMultiply(h ^ _.total_len, _.fold_seed ^ lenMix);

        if (_.quality) {
            const q1 = makeU64(0xE7037ED1 as u32, 0xA0B428DB as u32);
            const q2 = makeU64(0x8EBC6AF0 as u32, 0x9C88C6E3 as u32);
            h = foldedMultiply(h ^ q1, _.fold_seed ^ q2);
            h = foldedMultiply(h ^ (h / (0x100000000 as u64)), q1);
        }
        return h;
    }
}

pub fn hashBytesFast(data: []const u8, seed: u64) -> u64 {
    let h = FoldHasher.initFast(seed);
    h.writeBytes(data);
    return h.finish();
}

pub fn hashBytesQuality(data: []const u8, seed: u64) -> u64 {
    let h = FoldHasher.initQuality(seed);
    h.writeBytes(data);
    return h.finish();
}

pub fn hashU64(v: u64, seed: u64) -> u64 {
    let h = FoldHasher.initFast(seed);
    h.writeU64(v);
    return h.finish();
}

export {
    foldedMultiply,
    FoldHasher,
    hashBytesFast,
    hashBytesQuality,
    hashU64,
};
