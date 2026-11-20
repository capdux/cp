// Source: bun/src/collections/bit_set.cp -> cp/lib/collections/bitset.cp (Dynamic BitSet)
const std = @import("std");

pub struct BitSet {
    alloc: std.mem.Allocator,
    words: []u64,
    bit_len: usize,

    pub fn init(alloc: std.mem.Allocator, bit_len: usize): !BitSet {
        const num_words = (bit_len + 63) / 64;
        const words = try alloc.alloc(u64, num_words);
        @memset(words, 0);
        return BitSet{
            alloc: alloc,
            words: words,
            bit_len: bit_len,
        };
    }

    pub fn deinit(*_) {
        if (_.words.len > 0) {
            _.alloc.free(_.words);
        }
        _.words = [];
        _.bit_len = 0;
    }

    pub fn len(): usize {
        return _.bit_len;
    }

    pub fn set(*_, idx: usize) {
        if (idx >= _.bit_len) {
            return;
        }
        const w = idx >> 6;
        const b: u6 = @intCast(idx & 63);
        _.words[w] |= (@as(u64, 1) << b);
    }

    pub fn unset(*_, idx: usize) {
        if (idx >= _.bit_len) {
            return;
        }
        const w = idx >> 6;
        const b: u6 = @intCast(idx & 63);
        _.words[w] &= ~(@as(u64, 1) << b);
    }

    pub fn isSet(idx: usize): bool {
        if (idx >= _.bit_len) {
            return false;
        }
        const w = idx >> 6;
        const b: u6 = @intCast(idx & 63);
        return (_.words[w] & (@as(u64, 1) << b)) != 0;
    }

    pub fn toggle(*_, idx: usize) {
        if (idx >= _.bit_len) {
            return;
        }
        const w = idx >> 6;
        const b: u6 = @intCast(idx & 63);
        _.words[w] ^= (@as(u64, 1) << b);
    }

    pub fn clearAll(*_) {
        @memset(_.words, 0);
    }

    pub fn countSetBits(): usize {
        let total: usize = 0;
        for (w of _.words) {
            total += @popCount(w);
        }
        return total;
    }

    fn maskTail(*_) {
        if (_.words.len == 0) {
            return;
        }
        const rem = _.bit_len & 63;
        if (rem != 0) {
            const mask = (@as(u64, 1) << @as(u6, @intCast(rem))) - 1;
            _.words[_.words.len - 1] &= mask;
        }
    }

    pub fn unionWith(*_, other: *const BitSet) {
        const n = @min(_.words.len, other.words.len);
        for (i of 0..n) {
            _.words[i] |= other.words[i];
        }
        _.maskTail();
    }

    pub fn intersectWith(*_, other: *const BitSet) {
        const n = @min(_.words.len, other.words.len);
        for (i of 0..n) {
            _.words[i] &= other.words[i];
        }
        for (i of n.._.words.len) {
            _.words[i] = 0;
        }
    }

    pub fn differenceWith(*_, other: *const BitSet) {
        const n = @min(_.words.len, other.words.len);
        for (i of 0..n) {
            _.words[i] &= ~other.words[i];
        }
    }

    pub fn nextSetBit(start_idx: usize): usize? {
        if (start_idx >= _.bit_len) {
            return null;
        }
        let word_idx = start_idx >> 6;
        const bit_offset: u6 = @intCast(start_idx & 63);
        let w = _.words[word_idx] & (~@as(u64, 0) << bit_offset);
        while (true) {
            if (w != 0) {
                const tz: usize = @ctz(w);
                const res = (word_idx << 6) + tz;
                if (res < _.bit_len) {
                    return res;
                }
                return null;
            }
            word_idx += 1;
            if (word_idx >= _.words.len) {
                return null;
            }
            w = _.words[word_idx];
        }
    }
}
