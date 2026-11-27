// @cap/bitset — Zero-allocation fixed-size bitsets (BitSet256 / BitSet1024) for CP
// Clean-room MIT implementation inspired by Rust `fixedbitset` (634M), `bit-vec` (618M), and `bit-set` (557M).
// Accelerated with 4-lane `@Vector(4, u64)` 256-bit SIMD set algebra, comptime bit-mask & popcount LUTs,
// and word-bulk `setRange`.

type BitVec4u64 = @Vector(4, u64);

const ALL_ONES_U64: u64 = 0xFFFFFFFFFFFFFFFF as u64;

fn buildBitMaskLut() -> [64]u64 {
    @setEvalBranchQuota(20000);
    let lut: [64]u64 = undefined;
    let i: usize = 0;
    while (i < 64) {
        lut[i] = (1 as u64) << (i as u6);
        i += 1;
    }
    return lut;
}

fn buildPopcount8Lut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        let x: usize = i;
        let c: u8 = 0;
        while (x != 0) {
            c += (x & 1) as u8;
            x = x >> 1;
        }
        lut[i] = c;
        i += 1;
    }
    return lut;
}

const BIT_MASK_LUT: [64]u64 = buildBitMaskLut();
const POPCOUNT8_LUT: [256]u8 = buildPopcount8Lut();

fn popcount64(x: u64) -> usize {
    return (POPCOUNT8_LUT[(x & 0xFF) as usize] as usize)
        + (POPCOUNT8_LUT[((x >> 8) & 0xFF) as usize] as usize)
        + (POPCOUNT8_LUT[((x >> 16) & 0xFF) as usize] as usize)
        + (POPCOUNT8_LUT[((x >> 24) & 0xFF) as usize] as usize)
        + (POPCOUNT8_LUT[((x >> 32) & 0xFF) as usize] as usize)
        + (POPCOUNT8_LUT[((x >> 40) & 0xFF) as usize] as usize)
        + (POPCOUNT8_LUT[((x >> 48) & 0xFF) as usize] as usize)
        + (POPCOUNT8_LUT[((x >> 56) & 0xFF) as usize] as usize);
}

fn ctz64(w_in: u64) -> usize {
    let w = w_in;
    let tz: usize = 0;
    if ((w & (0xFFFFFFFF as u64)) == (0 as u64)) {
        tz += 32;
        w = w >> 32;
    }
    if ((w & (0xFFFF as u64)) == (0 as u64)) {
        tz += 16;
        w = w >> 16;
    }
    if ((w & (0xFF as u64)) == (0 as u64)) {
        tz += 8;
        w = w >> 8;
    }
    if ((w & (0xF as u64)) == (0 as u64)) {
        tz += 4;
        w = w >> 4;
    }
    if ((w & (0x3 as u64)) == (0 as u64)) {
        tz += 2;
        w = w >> 2;
    }
    if ((w & (0x1 as u64)) == (0 as u64)) {
        tz += 1;
    }
    return tz;
}

fn setRangeWords(words: []u64, start: usize, hi: usize) {
    if (start >= hi) {
        return;
    }
    const sw = start >> 6;
    const sb = start & 63;
    const ew = (hi - 1) >> 6;
    const eb = (hi - 1) & 63;
    const low_mask: u64 = ALL_ONES_U64 << (sb as u6);
    const high_mask: u64 = ALL_ONES_U64 >> ((63 - eb) as u6);
    if (sw == ew) {
        words[sw] = words[sw] | (low_mask & high_mask);
        return;
    }
    words[sw] = words[sw] | low_mask;
    let wi: usize = sw + 1;
    while (wi < ew) {
        words[wi] = ALL_ONES_U64;
        wi += 1;
    }
    words[ew] = words[ew] | high_mask;
}

pub struct BitSet256 {
    pub words: [4]u64,

    pub static fn init() -> BitSet256 {
        const z: BitVec4u64 = @splat(0 as u64);
        return BitSet256{ words: z };
    }

    pub static fn full() -> BitSet256 {
        const ones: BitVec4u64 = @splat(ALL_ONES_U64);
        return BitSet256{ words: ones };
    }

    pub fn len() -> usize {
        return 256 as usize;
    }

    pub fn clear(*_) {
        const z: BitVec4u64 = @splat(0 as u64);
        _.words = z;
    }

    pub fn contains(bit: usize) -> bool {
        if (bit >= 256) {
            return false;
        }
        return (_.words[bit >> 6] & BIT_MASK_LUT[bit & 63]) != (0 as u64);
    }

    pub fn insert(*_, bit: usize) -> bool {
        if (bit >= 256) {
            return false;
        }
        const wi = bit >> 6;
        const mask = BIT_MASK_LUT[bit & 63];
        const already = (_.words[wi] & mask) != (0 as u64);
        _.words[wi] = _.words[wi] | mask;
        return !already;
    }

    pub fn remove(*_, bit: usize) -> bool {
        if (bit >= 256) {
            return false;
        }
        const wi = bit >> 6;
        const mask = BIT_MASK_LUT[bit & 63];
        const present = (_.words[wi] & mask) != (0 as u64);
        if (present) {
            _.words[wi] = _.words[wi] ^ mask;
        }
        return present;
    }

    pub fn toggle(*_, bit: usize) {
        if (bit >= 256) {
            return;
        }
        const wi = bit >> 6;
        _.words[wi] = _.words[wi] ^ BIT_MASK_LUT[bit & 63];
    }

    pub fn setRange(*_, start: usize, end: usize) {
        const hi = if (end > 256) { 256 as usize } else { end };
        setRangeWords(_.words[0..4], start, hi);
    }

    pub fn countOnes() -> usize {
        return popcount64(_.words[0]) + popcount64(_.words[1]) + popcount64(_.words[2]) + popcount64(_.words[3]);
    }

    pub fn isEmpty() -> bool {
        const v: BitVec4u64 = _.words;
        return @reduce(.Or, v) == (0 as u64);
    }

    pub fn firstOne() -> usize? {
        return _.nextOne(0 as usize);
    }

    pub fn nextOne(from_bit: usize) -> usize? {
        if (from_bit >= 256) {
            return null;
        }
        let wi = from_bit >> 6;
        const bi = from_bit & 63;
        if (bi > 0) {
            const shifted = _.words[wi] >> (bi as u6);
            if (shifted != (0 as u64)) {
                return (wi << 6) + bi + ctz64(shifted);
            }
            wi += 1;
        }
        while (wi < 4) {
            if (_.words[wi] != (0 as u64)) {
                return (wi << 6) + ctz64(_.words[wi]);
            }
            wi += 1;
        }
        return null;
    }

    pub fn unionWith(*_, other: BitSet256) {
        const va: BitVec4u64 = _.words;
        const vb: BitVec4u64 = other.words;
        _.words = va | vb;
    }

    pub fn intersectWith(*_, other: BitSet256) {
        const va: BitVec4u64 = _.words;
        const vb: BitVec4u64 = other.words;
        _.words = va & vb;
    }

    pub fn differenceWith(*_, other: BitSet256) {
        const va: BitVec4u64 = _.words;
        const vb: BitVec4u64 = other.words;
        _.words = va & (va ^ vb);
    }

    pub fn symmetricDiffWith(*_, other: BitSet256) {
        const va: BitVec4u64 = _.words;
        const vb: BitVec4u64 = other.words;
        _.words = va ^ vb;
    }

    pub fn isSubsetOf(other: BitSet256) -> bool {
        const va: BitVec4u64 = _.words;
        const vb: BitVec4u64 = other.words;
        return @reduce(.Or, va & (va ^ vb)) == (0 as u64);
    }

    pub fn isDisjoint(other: BitSet256) -> bool {
        const va: BitVec4u64 = _.words;
        const vb: BitVec4u64 = other.words;
        return @reduce(.Or, va & vb) == (0 as u64);
    }

    pub fn equals(other: BitSet256) -> bool {
        const va: BitVec4u64 = _.words;
        const vb: BitVec4u64 = other.words;
        return @reduce(.Or, va ^ vb) == (0 as u64);
    }
}

pub struct BitSet1024 {
    pub words: [16]u64,

    pub static fn init() -> BitSet1024 {
        let w: [16]u64 = undefined;
        const zv: BitVec4u64 = @splat(0 as u64);
        const z: [4]u64 = zv;
        let i: usize = 0;
        while (i < 16) {
            @memcpy(w[i .. i + 4], z[0..4]);
            i += 4;
        }
        return BitSet1024{ words: w };
    }

    pub static fn full() -> BitSet1024 {
        let w: [16]u64 = undefined;
        const ones_v: BitVec4u64 = @splat(ALL_ONES_U64);
        const ones: [4]u64 = ones_v;
        let i: usize = 0;
        while (i < 16) {
            @memcpy(w[i .. i + 4], ones[0..4]);
            i += 4;
        }
        return BitSet1024{ words: w };
    }

    pub fn len() -> usize {
        return 1024 as usize;
    }

    pub fn clear(*_) {
        const zv: BitVec4u64 = @splat(0 as u64);
        const z: [4]u64 = zv;
        let i: usize = 0;
        while (i < 16) {
            @memcpy(_.words[i .. i + 4], z[0..4]);
            i += 4;
        }
    }

    pub fn contains(bit: usize) -> bool {
        if (bit >= 1024) {
            return false;
        }
        return (_.words[bit >> 6] & BIT_MASK_LUT[bit & 63]) != (0 as u64);
    }

    pub fn insert(*_, bit: usize) -> bool {
        if (bit >= 1024) {
            return false;
        }
        const wi = bit >> 6;
        const mask = BIT_MASK_LUT[bit & 63];
        const already = (_.words[wi] & mask) != (0 as u64);
        _.words[wi] = _.words[wi] | mask;
        return !already;
    }

    pub fn remove(*_, bit: usize) -> bool {
        if (bit >= 1024) {
            return false;
        }
        const wi = bit >> 6;
        const mask = BIT_MASK_LUT[bit & 63];
        const present = (_.words[wi] & mask) != (0 as u64);
        if (present) {
            _.words[wi] = _.words[wi] ^ mask;
        }
        return present;
    }

    pub fn toggle(*_, bit: usize) {
        if (bit >= 1024) {
            return;
        }
        const wi = bit >> 6;
        _.words[wi] = _.words[wi] ^ BIT_MASK_LUT[bit & 63];
    }

    pub fn setRange(*_, start: usize, end: usize) {
        const hi = if (end > 1024) { 1024 as usize } else { end };
        setRangeWords(_.words[0..16], start, hi);
    }

    pub fn countOnes() -> usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < 16) {
            total += popcount64(_.words[i]);
            i += 1;
        }
        return total;
    }

    pub fn isEmpty() -> bool {
        const v0: BitVec4u64 = _.words[0..4][0..4].*;
        const v1: BitVec4u64 = _.words[4..8][0..4].*;
        const v2: BitVec4u64 = _.words[8..12][0..4].*;
        const v3: BitVec4u64 = _.words[12..16][0..4].*;
        return @reduce(.Or, (v0 | v1) | (v2 | v3)) == (0 as u64);
    }

    pub fn firstOne() -> usize? {
        return _.nextOne(0 as usize);
    }

    pub fn nextOne(from_bit: usize) -> usize? {
        if (from_bit >= 1024) {
            return null;
        }
        let wi = from_bit >> 6;
        const bi = from_bit & 63;
        if (bi > 0) {
            const shifted = _.words[wi] >> (bi as u6);
            if (shifted != (0 as u64)) {
                return (wi << 6) + bi + ctz64(shifted);
            }
            wi += 1;
        }
        while (wi < 16) {
            if (_.words[wi] != (0 as u64)) {
                return (wi << 6) + ctz64(_.words[wi]);
            }
            wi += 1;
        }
        return null;
    }

    pub fn unionWith(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            const va: BitVec4u64 = _.words[i .. i + 4][0..4].*;
            const vb: BitVec4u64 = other.words[i .. i + 4][0..4].*;
            const out: [4]u64 = va | vb;
            @memcpy(_.words[i .. i + 4], out[0..4]);
            i += 4;
        }
    }

    pub fn intersectWith(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            const va: BitVec4u64 = _.words[i .. i + 4][0..4].*;
            const vb: BitVec4u64 = other.words[i .. i + 4][0..4].*;
            const out: [4]u64 = va & vb;
            @memcpy(_.words[i .. i + 4], out[0..4]);
            i += 4;
        }
    }

    pub fn differenceWith(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            const va: BitVec4u64 = _.words[i .. i + 4][0..4].*;
            const vb: BitVec4u64 = other.words[i .. i + 4][0..4].*;
            const out: [4]u64 = va & (va ^ vb);
            @memcpy(_.words[i .. i + 4], out[0..4]);
            i += 4;
        }
    }

    pub fn symmetricDiffWith(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            const va: BitVec4u64 = _.words[i .. i + 4][0..4].*;
            const vb: BitVec4u64 = other.words[i .. i + 4][0..4].*;
            const out: [4]u64 = va ^ vb;
            @memcpy(_.words[i .. i + 4], out[0..4]);
            i += 4;
        }
    }

    pub fn isSubsetOf(other: BitSet1024) -> bool {
        let acc: BitVec4u64 = @splat(0 as u64);
        let i: usize = 0;
        while (i < 16) {
            const va: BitVec4u64 = _.words[i .. i + 4][0..4].*;
            const vb: BitVec4u64 = other.words[i .. i + 4][0..4].*;
            acc = acc | (va & (va ^ vb));
            i += 4;
        }
        return @reduce(.Or, acc) == (0 as u64);
    }

    pub fn isDisjoint(other: BitSet1024) -> bool {
        let acc: BitVec4u64 = @splat(0 as u64);
        let i: usize = 0;
        while (i < 16) {
            const va: BitVec4u64 = _.words[i .. i + 4][0..4].*;
            const vb: BitVec4u64 = other.words[i .. i + 4][0..4].*;
            acc = acc | (va & vb);
            i += 4;
        }
        return @reduce(.Or, acc) == (0 as u64);
    }

    pub fn equals(other: BitSet1024) -> bool {
        let acc: BitVec4u64 = @splat(0 as u64);
        let i: usize = 0;
        while (i < 16) {
            const va: BitVec4u64 = _.words[i .. i + 4][0..4].*;
            const vb: BitVec4u64 = other.words[i .. i + 4][0..4].*;
            acc = acc | (va ^ vb);
            i += 4;
        }
        return @reduce(.Or, acc) == (0 as u64);
    }
}

export {
    BitSet256,
    BitSet1024,
};
