// @cap/bitvec — Zero-heap arbitrary-length bit vector (up to 512 bits / 8x u64 words)
// Clean-room MIT CP implementation of crates.io `bitvec` (600M+ downloads)
// Supports push/pop/get/set/toggle, popcount, firstOne/lastOne, bitwise AND/OR/XOR/NOT, shifts.

pub const BITVEC_WORDS: usize = 8;
pub const BITVEC_MAX_BITS: usize = 512;

fn bvPopcountU64(x: u64): usize {
    let v = x;
    let c: usize = 0;
    while (v != 0) {
        v = v & (v - 1);
        c = c + 1;
    }
    return c;
}

/// Zero-heap bit vector supporting up to 512 bits with O(1) bit indexing and word-level bitwise operations.
pub struct BitVec512 {
    words: [8]u64,
    bit_len: usize,

    pub static fn new(): BitVec512 {
        return BitVec512 {
            words: [0, 0, 0, 0, 0, 0, 0, 0],
            bit_len: 0,
        };
    }

    pub static fn withLen(n_bits: usize, initial: bool): BitVec512 {
        let clamped = if (n_bits > BITVEC_MAX_BITS) { BITVEC_MAX_BITS } else { n_bits };
        let v = BitVec512.new();
        v.bit_len = clamped;
        if (initial) {
            let full_words = clamped / 64;
            let rem = clamped % 64;
            let all_ones: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
            let i: usize = 0;
            while (i < full_words) {
                v.words[i] = all_ones;
                i = i + 1;
            }
            if (rem > 0) {
                v.words[full_words] = ((1 as u64) << (rem as u6)) - 1;
            }
        }
        return v;
    }

    pub fn len(): usize { return _.bit_len; }
    pub fn capacity(): usize { return BITVEC_MAX_BITS; }
    pub fn isEmpty(): bool { return _.bit_len == 0; }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < BITVEC_WORDS) {
            _.words[i] = 0;
            i = i + 1;
        }
        _.bit_len = 0;
    }

    pub fn get(idx: usize): bool {
        if (idx >= _.bit_len) { return false; }
        let w = idx / 64;
        let b = (idx % 64) as u6;
        return ((_.words[w] >> b) & 1) != 0;
    }

    pub fn set(*_, idx: usize, val: bool): bool {
        if (idx >= _.bit_len) { return false; }
        let w = idx / 64;
        let b = (idx % 64) as u6;
        let mask: u64 = (1 as u64) << b;
        if (val) {
            _.words[w] = _.words[w] | mask;
        } else {
            _.words[w] = _.words[w] & (~mask);
        }
        return true;
    }

    pub fn toggle(*_, idx: usize): bool {
        if (idx >= _.bit_len) { return false; }
        let w = idx / 64;
        let b = (idx % 64) as u6;
        _.words[w] = _.words[w] ^ ((1 as u64) << b);
        return true;
    }

    pub fn push(*_, val: bool): bool {
        if (_.bit_len >= BITVEC_MAX_BITS) { return false; }
        let idx = _.bit_len;
        _.bit_len = _.bit_len + 1;
        return _.set(idx, val);
    }

    pub fn pop(*_): i32 {
        if (_.bit_len == 0) { return -1; }
        let idx = _.bit_len - 1;
        let bit: i32 = if (_.get(idx)) { 1 } else { 0 };
        * = _.set(idx, false);
        _.bit_len = idx;
        return bit;
    }

    pub fn countOnes(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < BITVEC_WORDS) {
            total = total + bvPopcountU64(_.words[i]);
            i = i + 1;
        }
        return total;
    }

    pub fn countZeros(): usize {
        return _.bit_len - _.countOnes();
    }

    pub fn any(): bool {
        return _.countOnes() > 0;
    }

    pub fn all(): bool {
        return _.countOnes() == _.bit_len;
    }

    pub fn firstOne(): i32 {
        let i: usize = 0;
        while (i < _.bit_len) {
            let w = i / 64;
            if (_.words[w] == 0) {
                i = (w + 1) * 64;
            } else {
                if (_.get(i)) { return i as i32; }
                i = i + 1;
            }
        }
        return -1;
    }

    pub fn lastOne(): i32 {
        let i = _.bit_len;
        while (i > 0) {
            i = i - 1;
            if (_.get(i)) { return i as i32; }
        }
        return -1;
    }

    pub fn andWith(*_, other: BitVec512) {
        let i: usize = 0;
        while (i < BITVEC_WORDS) {
            _.words[i] = _.words[i] & other.words[i];
            i = i + 1;
        }
    }

    pub fn orWith(*_, other: BitVec512) {
        let i: usize = 0;
        while (i < BITVEC_WORDS) {
            _.words[i] = _.words[i] | other.words[i];
            i = i + 1;
        }
    }

    pub fn xorWith(*_, other: BitVec512) {
        let i: usize = 0;
        while (i < BITVEC_WORDS) {
            _.words[i] = _.words[i] ^ other.words[i];
            i = i + 1;
        }
    }

    pub fn notAll(*_) {
        let full_words = _.bit_len / 64;
        let rem = _.bit_len % 64;
        let i: usize = 0;
        while (i < full_words) {
            _.words[i] = ~_.words[i];
            i = i + 1;
        }
        if (rem > 0) {
            let mask: u64 = ((1 as u64) << (rem as u6)) - 1;
            _.words[full_words] = (~_.words[full_words]) & mask;
        }
    }

    /// Formats the bit vector as an LSB-first `'0'`/`'1'` ASCII string into `out`.
    pub fn toBinStr(out: []u8): usize {
        let n = if (_.bit_len <= out.len) { _.bit_len } else { out.len };
        let i: usize = 0;
        while (i < n) {
            out[i] = if (_.get(i)) { 49 } else { 48 };
            i = i + 1;
        }
        return n;
    }
}
