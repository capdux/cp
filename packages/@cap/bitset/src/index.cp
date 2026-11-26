// @cap/bitset — Zero-allocation fixed-size bitsets (BitSet256 / BitSet1024) for CP
// Clean-room MIT implementation inspired by Rust `fixedbitset` (634M), `bit-vec` (618M), and `bit-set` (557M).

fn allOnes64() -> u64 {
    return ((0xFFFFFFFF as u64) * (0x100000000 as u64)) | (0xFFFFFFFF as u64);
}

fn bitMask64(bit: usize) -> u64 {
    let v: u64 = 1;
    let i: usize = 0;
    while (i < bit) {
        v = v * (2 as u64);
        i += 1;
    }
    return v;
}

fn popcount64(x_in: u64) -> usize {
    let x = x_in;
    x = x - ((x / (2 as u64)) & (0x5555555555555555 as u64));
    x = (x & (0x3333333333333333 as u64)) + ((x / (4 as u64)) & (0x3333333333333333 as u64));
    x = (x + (x / (16 as u64))) & (0x0F0F0F0F0F0F0F0F as u64);
    let total: usize = 0;
    let i: usize = 0;
    while (i < 8) {
        total += (x & (0xFF as u64)) as usize;
        x = x / (256 as u64);
        i += 1;
    }
    return total;
}

fn ctz64(w_in: u64) -> usize {
    let w = w_in;
    let tz: usize = 0;
    if ((w & (0xFFFFFFFF as u64)) == (0 as u64)) {
        tz += 32;
        w = w / (0x100000000 as u64);
    }
    if ((w & (0xFFFF as u64)) == (0 as u64)) {
        tz += 16;
        w = w / (0x10000 as u64);
    }
    if ((w & (0xFF as u64)) == (0 as u64)) {
        tz += 8;
        w = w / (256 as u64);
    }
    if ((w & (0xF as u64)) == (0 as u64)) {
        tz += 4;
        w = w / (16 as u64);
    }
    if ((w & (0x3 as u64)) == (0 as u64)) {
        tz += 2;
        w = w / (4 as u64);
    }
    if ((w & (0x1 as u64)) == (0 as u64)) {
        tz += 1;
    }
    return tz;
}

pub struct BitSet256 {
    pub words: [4]u64,

    pub static fn init() -> BitSet256 {
        let w: [4]u64 = undefined;
        w[0] = 0 as u64;
        w[1] = 0 as u64;
        w[2] = 0 as u64;
        w[3] = 0 as u64;
        return BitSet256{ words: w };
    }

    pub static fn full() -> BitSet256 {
        const ones = allOnes64();
        let w: [4]u64 = undefined;
        w[0] = ones;
        w[1] = ones;
        w[2] = ones;
        w[3] = ones;
        return BitSet256{ words: w };
    }

    pub fn len() -> usize {
        return 256 as usize;
    }

    pub fn clear(*_) {
        _.words[0] = 0 as u64;
        _.words[1] = 0 as u64;
        _.words[2] = 0 as u64;
        _.words[3] = 0 as u64;
    }

    pub fn contains(bit: usize) -> bool {
        if (bit >= 256) {
            return false;
        }
        const wi = bit / (64 as usize);
        const bi = bit % (64 as usize);
        return (_.words[wi] & bitMask64(bi)) != (0 as u64);
    }

    pub fn insert(*_, bit: usize) -> bool {
        if (bit >= 256) {
            return false;
        }
        const wi = bit / (64 as usize);
        const bi = bit % (64 as usize);
        const mask = bitMask64(bi);
        const already = (_.words[wi] & mask) != (0 as u64);
        _.words[wi] = _.words[wi] | mask;
        return !already;
    }

    pub fn remove(*_, bit: usize) -> bool {
        if (bit >= 256) {
            return false;
        }
        const wi = bit / (64 as usize);
        const bi = bit % (64 as usize);
        const mask = bitMask64(bi);
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
        const wi = bit / (64 as usize);
        const bi = bit % (64 as usize);
        _.words[wi] = _.words[wi] ^ bitMask64(bi);
    }

    pub fn setRange(*_, start: usize, end: usize) {
        const hi = if (end > 256) { 256 as usize } else { end };
        let b = start;
        while (b < hi) {
            const wi = b / (64 as usize);
            const bi = b % (64 as usize);
            _.words[wi] = _.words[wi] | bitMask64(bi);
            b += 1;
        }
    }

    pub fn countOnes() -> usize {
        return popcount64(_.words[0]) + popcount64(_.words[1]) + popcount64(_.words[2]) + popcount64(_.words[3]);
    }

    pub fn isEmpty() -> bool {
        return (_.words[0] | _.words[1] | _.words[2] | _.words[3]) == (0 as u64);
    }

    pub fn firstOne() -> usize? {
        return _.nextOne(0 as usize);
    }

    pub fn nextOne(from_bit: usize) -> usize? {
        if (from_bit >= 256) {
            return null;
        }
        let wi = from_bit / (64 as usize);
        const bi = from_bit % (64 as usize);
        if (bi > 0) {
            const shifted = _.words[wi] / bitMask64(bi);
            if (shifted != (0 as u64)) {
                return (wi * (64 as usize)) + bi + ctz64(shifted);
            }
            wi += 1;
        }
        while (wi < 4) {
            if (_.words[wi] != (0 as u64)) {
                return (wi * (64 as usize)) + ctz64(_.words[wi]);
            }
            wi += 1;
        }
        return null;
    }

    pub fn unionWith(*_, other: BitSet256) {
        let i: usize = 0;
        while (i < 4) {
            _.words[i] = _.words[i] | other.words[i];
            i += 1;
        }
    }

    pub fn intersectWith(*_, other: BitSet256) {
        let i: usize = 0;
        while (i < 4) {
            _.words[i] = _.words[i] & other.words[i];
            i += 1;
        }
    }

    pub fn differenceWith(*_, other: BitSet256) {
        let i: usize = 0;
        while (i < 4) {
            _.words[i] = _.words[i] & (_.words[i] ^ other.words[i]);
            i += 1;
        }
    }

    pub fn symmetricDiffWith(*_, other: BitSet256) {
        let i: usize = 0;
        while (i < 4) {
            _.words[i] = _.words[i] ^ other.words[i];
            i += 1;
        }
    }

    pub fn isSubsetOf(other: BitSet256) -> bool {
        let i: usize = 0;
        while (i < 4) {
            if ((_.words[i] & other.words[i]) != _.words[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn isDisjoint(other: BitSet256) -> bool {
        let i: usize = 0;
        while (i < 4) {
            if ((_.words[i] & other.words[i]) != (0 as u64)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn equals(other: BitSet256) -> bool {
        let i: usize = 0;
        while (i < 4) {
            if (_.words[i] != other.words[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

pub struct BitSet1024 {
    pub words: [16]u64,

    pub static fn init() -> BitSet1024 {
        let w: [16]u64 = undefined;
        let i: usize = 0;
        while (i < 16) {
            w[i] = 0 as u64;
            i += 1;
        }
        return BitSet1024{ words: w };
    }

    pub static fn full() -> BitSet1024 {
        const ones = allOnes64();
        let w: [16]u64 = undefined;
        let i: usize = 0;
        while (i < 16) {
            w[i] = ones;
            i += 1;
        }
        return BitSet1024{ words: w };
    }

    pub fn len() -> usize {
        return 1024 as usize;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < 16) {
            _.words[i] = 0 as u64;
            i += 1;
        }
    }

    pub fn contains(bit: usize) -> bool {
        if (bit >= 1024) {
            return false;
        }
        const wi = bit / (64 as usize);
        const bi = bit % (64 as usize);
        return (_.words[wi] & bitMask64(bi)) != (0 as u64);
    }

    pub fn insert(*_, bit: usize) -> bool {
        if (bit >= 1024) {
            return false;
        }
        const wi = bit / (64 as usize);
        const bi = bit % (64 as usize);
        const mask = bitMask64(bi);
        const already = (_.words[wi] & mask) != (0 as u64);
        _.words[wi] = _.words[wi] | mask;
        return !already;
    }

    pub fn remove(*_, bit: usize) -> bool {
        if (bit >= 1024) {
            return false;
        }
        const wi = bit / (64 as usize);
        const bi = bit % (64 as usize);
        const mask = bitMask64(bi);
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
        const wi = bit / (64 as usize);
        const bi = bit % (64 as usize);
        _.words[wi] = _.words[wi] ^ bitMask64(bi);
    }

    pub fn setRange(*_, start: usize, end: usize) {
        const hi = if (end > 1024) { 1024 as usize } else { end };
        let b = start;
        while (b < hi) {
            const wi = b / (64 as usize);
            const bi = b % (64 as usize);
            _.words[wi] = _.words[wi] | bitMask64(bi);
            b += 1;
        }
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
        let i: usize = 0;
        while (i < 16) {
            if (_.words[i] != (0 as u64)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn firstOne() -> usize? {
        return _.nextOne(0 as usize);
    }

    pub fn nextOne(from_bit: usize) -> usize? {
        if (from_bit >= 1024) {
            return null;
        }
        let wi = from_bit / (64 as usize);
        const bi = from_bit % (64 as usize);
        if (bi > 0) {
            const shifted = _.words[wi] / bitMask64(bi);
            if (shifted != (0 as u64)) {
                return (wi * (64 as usize)) + bi + ctz64(shifted);
            }
            wi += 1;
        }
        while (wi < 16) {
            if (_.words[wi] != (0 as u64)) {
                return (wi * (64 as usize)) + ctz64(_.words[wi]);
            }
            wi += 1;
        }
        return null;
    }

    pub fn unionWith(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            _.words[i] = _.words[i] | other.words[i];
            i += 1;
        }
    }

    pub fn intersectWith(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            _.words[i] = _.words[i] & other.words[i];
            i += 1;
        }
    }

    pub fn differenceWith(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            _.words[i] = _.words[i] & (_.words[i] ^ other.words[i]);
            i += 1;
        }
    }

    pub fn symmetricDiffWith(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            _.words[i] = _.words[i] ^ other.words[i];
            i += 1;
        }
    }

    pub fn isSubsetOf(other: BitSet1024) -> bool {
        let i: usize = 0;
        while (i < 16) {
            if ((_.words[i] & other.words[i]) != _.words[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn isDisjoint(other: BitSet1024) -> bool {
        let i: usize = 0;
        while (i < 16) {
            if ((_.words[i] & other.words[i]) != (0 as u64)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn equals(other: BitSet1024) -> bool {
        let i: usize = 0;
        while (i < 16) {
            if (_.words[i] != other.words[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

export {
    BitSet256,
    BitSet1024,
};
