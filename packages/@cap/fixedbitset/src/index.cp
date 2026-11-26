// @cap/fixedbitset — Zero-heap 1024-bit FixedBitSet modeled on Rust's `fixedbitset` crate.
// Clean-room MIT implementation using 16 x u64 words with O(1) bit ops & O(W) set algebra.

pub const FBS_MAX_BITS: usize = 1024;
pub const FBS_WORDS: usize = 16;

fn fbsPopcount64(x: u64): usize {
    let v = x;
    let c: usize = 0;
    while (v != 0) {
        v = v & (v - 1);
        c = c + 1;
    }
    return c;
}

fn fbsCtz64(x: u64): usize {
    if (x == 0) { return 64; }
    let v = x;
    let n: usize = 0;
    while ((v & 1) == 0) {
        v = v >> 1;
        n = n + 1;
    }
    return n;
}

pub struct FixedBitSet {
    pub words: [16]u64,
    pub bit_len: usize,

    pub static fn withCapacity(bits: usize): FixedBitSet {
        let clamped = if (bits > FBS_MAX_BITS) { FBS_MAX_BITS } else { bits };
        let w: [16]u64 = undefined;
        let i: usize = 0;
        while (i < FBS_WORDS) {
            w[i] = 0;
            i = i + 1;
        }
        return FixedBitSet { words: w, bit_len: clamped };
    }

    pub fn len(): usize {
        return _.bit_len;
    }

    pub fn isEmpty(): bool {
        return _.bit_len == 0;
    }

    pub fn contains(bit: usize): bool {
        if (bit >= _.bit_len) { return false; }
        let wi = bit >> 6;
        let sh: u6 = (bit & 63) as u6;
        return ((_.words[wi] >> sh) & 1) != 0;
    }

    pub fn insert(*_, bit: usize): bool {
        if (bit >= _.bit_len) { return false; }
        let wi = bit >> 6;
        let sh: u6 = (bit & 63) as u6;
        let mask: u64 = (1 as u64) << sh;
        let prev = (_.words[wi] & mask) != 0;
        _.words[wi] = _.words[wi] | mask;
        return !prev;
    }

    pub fn put(*_, bit: usize): bool {
        if (bit >= _.bit_len) { return false; }
        let wi = bit >> 6;
        let sh: u6 = (bit & 63) as u6;
        let mask: u64 = (1 as u64) << sh;
        let prev = (_.words[wi] & mask) != 0;
        _.words[wi] = _.words[wi] | mask;
        return prev;
    }

    pub fn remove(*_, bit: usize): bool {
        if (bit >= _.bit_len) { return false; }
        let wi = bit >> 6;
        let sh: u6 = (bit & 63) as u6;
        let mask: u64 = (1 as u64) << sh;
        let prev = (_.words[wi] & mask) != 0;
        _.words[wi] = _.words[wi] & (~mask);
        return prev;
    }

    pub fn toggle(*_, bit: usize) {
        if (bit >= _.bit_len) { return; }
        let wi = bit >> 6;
        let sh: u6 = (bit & 63) as u6;
        _.words[wi] = _.words[wi] ^ ((1 as u64) << sh);
    }

    pub fn set(*_, bit: usize, enabled: bool) {
        if (enabled) { _.insert(bit); } else { _.remove(bit); }
    }

    pub fn insertRange(*_, start: usize, end: usize) {
        let hi = if (end > _.bit_len) { _.bit_len } else { end };
        let i = start;
        while (i < hi) {
            _.insert(i);
            i = i + 1;
        }
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < FBS_WORDS) {
            _.words[i] = 0;
            i = i + 1;
        }
    }

    pub fn countOnes(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < FBS_WORDS) {
            total = total + fbsPopcount64(_.words[i]);
            i = i + 1;
        }
        return total;
    }

    pub fn isClear(): bool {
        let i: usize = 0;
        while (i < FBS_WORDS) {
            if (_.words[i] != 0) { return false; }
            i = i + 1;
        }
        return true;
    }

    pub fn unionWith(*_, other: FixedBitSet) {
        let i: usize = 0;
        while (i < FBS_WORDS) {
            _.words[i] = _.words[i] | other.words[i];
            i = i + 1;
        }
        _.maskTail();
    }

    pub fn intersectWith(*_, other: FixedBitSet) {
        let i: usize = 0;
        while (i < FBS_WORDS) {
            _.words[i] = _.words[i] & other.words[i];
            i = i + 1;
        }
    }

    pub fn differenceWith(*_, other: FixedBitSet) {
        let i: usize = 0;
        while (i < FBS_WORDS) {
            _.words[i] = _.words[i] & (~other.words[i]);
            i = i + 1;
        }
    }

    pub fn symmetricDifferenceWith(*_, other: FixedBitSet) {
        let i: usize = 0;
        while (i < FBS_WORDS) {
            _.words[i] = _.words[i] ^ other.words[i];
            i = i + 1;
        }
        _.maskTail();
    }

    pub fn isSubset(other: FixedBitSet): bool {
        let i: usize = 0;
        while (i < FBS_WORDS) {
            if ((_.words[i] & (~other.words[i])) != 0) { return false; }
            i = i + 1;
        }
        return true;
    }

    pub fn isDisjoint(other: FixedBitSet): bool {
        let i: usize = 0;
        while (i < FBS_WORDS) {
            if ((_.words[i] & other.words[i]) != 0) { return false; }
            i = i + 1;
        }
        return true;
    }

    /// Finds the next set bit index >= `from_bit`, or `null` if none.
    pub fn nextOne(from_bit: usize): usize? {
        if (from_bit >= _.bit_len) { return null; }
        let wi = from_bit >> 6;
        let off: u6 = (from_bit & 63) as u6;
        let w = _.words[wi] >> off;
        if (w != 0) {
            let idx = from_bit + fbsCtz64(w);
            if (idx < _.bit_len) { return idx; }
            return null;
        }
        let k = wi + 1;
        while (k < FBS_WORDS) {
            if (_.words[k] != 0) {
                let idx = (k << 6) + fbsCtz64(_.words[k]);
                if (idx < _.bit_len) { return idx; }
                return null;
            }
            k = k + 1;
        }
        return null;
    }

    fn maskTail(*_) {
        let full_words = _.bit_len >> 6;
        let rem = _.bit_len & 63;
        if (full_words < FBS_WORDS) {
            if (rem == 0) {
                _.words[full_words] = 0;
            } else {
                let sh: u6 = rem as u6;
                let mask: u64 = ((1 as u64) << sh) - 1;
                _.words[full_words] = _.words[full_words] & mask;
            }
            let k = full_words + 1;
            while (k < FBS_WORDS) {
                _.words[k] = 0;
                k = k + 1;
            }
        }
    }
}
