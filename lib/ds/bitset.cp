// Source: native-cp-stdlib (Word-Accelerated BitSet)

import * as std from "std";

export error BitSetError {
    OutOfBounds,
    SizeMismatch,
    InvalidRange,
}

export struct BitSet {
    words: []u64,
    bit_len: usize,
    alloc: std.mem.Allocator,

    static fn wordCount(bit_len: usize) -> usize {
        if (bit_len == 0) {
            return 0;
        }
        return ((bit_len - 1) / 64) + 1;
    }

    fn maskLastWord(*_) {
        if (_.bit_len == 0 or _.words.len == 0) {
            return;
        }
        const rem: usize = _.bit_len % 64;
        if (rem != 0) {
            const shift: u6 = @intCast(rem);
            const mask: u64 = (@as(u64, 1) << shift) - 1;
            const last_idx: usize = _.words.len - 1;
            _.words[last_idx] = _.words[last_idx] & mask;
        }
    }

    pub fn initEmpty(alloc: std.mem.Allocator, bit_len: usize) -> !BitSet {
        const wc: usize = BitSet.wordCount(bit_len);
        let words = alloc.alloc(u64, wc)?;
        @memset(words, 0);
        return BitSet {
            words: words,
            bit_len: bit_len,
            alloc: alloc,
        };
    }

    pub fn initFull(alloc: std.mem.Allocator, bit_len: usize) -> !BitSet {
        const wc: usize = BitSet.wordCount(bit_len);
        let words = alloc.alloc(u64, wc)?;
        @memset(words, 0xFFFFFFFFFFFFFFFF);
        let bs = BitSet {
            words: words,
            bit_len: bit_len,
            alloc: alloc,
        };
        bs.maskLastWord();
        return bs;
    }

    pub fn deinit(*_) {
        if (_.words.len > 0) {
            _.alloc.free(_.words);
        }
        _.words = [];
        _.bit_len = 0;
    }

    pub fn set(*_, idx: usize) -> !void {
        if (idx >= _.bit_len) {
            return BitSetError.OutOfBounds;
        }
        const w_idx: usize = idx >> 6;
        const b_idx: u6 = @intCast(idx & 63);
        _.words[w_idx] = _.words[w_idx] | (@as(u64, 1) << b_idx);
    }

    pub fn unset(*_, idx: usize) -> !void {
        if (idx >= _.bit_len) {
            return BitSetError.OutOfBounds;
        }
        const w_idx: usize = idx >> 6;
        const b_idx: u6 = @intCast(idx & 63);
        _.words[w_idx] = _.words[w_idx] & ~(@as(u64, 1) << b_idx);
    }

    pub fn toggle(*_, idx: usize) -> !void {
        if (idx >= _.bit_len) {
            return BitSetError.OutOfBounds;
        }
        const w_idx: usize = idx >> 6;
        const b_idx: u6 = @intCast(idx & 63);
        _.words[w_idx] = _.words[w_idx] ^ (@as(u64, 1) << b_idx);
    }

    pub fn isSet(idx: usize) -> bool {
        if (idx >= _.bit_len) {
            return false;
        }
        const w_idx: usize = idx >> 6;
        const b_idx: u6 = @intCast(idx & 63);
        return ((_.words[w_idx] >> b_idx) & 1) != 0;
    }

    pub fn setRange(*_, start: usize, end: usize) -> !void {
        if (start > end) {
            return BitSetError.InvalidRange;
        }
        if (end > _.bit_len) {
            return BitSetError.OutOfBounds;
        }
        if (start == end) {
            return;
        }

        const start_word: usize = start >> 6;
        const end_bit: usize = end - 1;
        const end_word: usize = end_bit >> 6;
        const start_off: u6 = @intCast(start & 63);
        const end_off: u6 = @intCast(end_bit & 63);
        const inv_end_off: u6 = 63 - end_off;

        const all_ones: u64 = 0xFFFFFFFFFFFFFFFF;
        const start_mask: u64 = all_ones << start_off;
        const end_mask: u64 = all_ones >> inv_end_off;

        if (start_word == end_word) {
            _.words[start_word] = _.words[start_word] | (start_mask & end_mask);
            return;
        }

        _.words[start_word] = _.words[start_word] | start_mask;
        let w: usize = start_word + 1;
        while (w < end_word) {
            _.words[w] = all_ones;
            w = w + 1;
        }
        _.words[end_word] = _.words[end_word] | end_mask;
    }

    pub fn count() -> usize {
        let total: usize = 0;
        for (w of _.words) {
            total = total + @as(usize, @popCount(w));
        }
        return total;
    }

    pub fn nextSetBit(from_idx: usize) -> usize? {
        if (from_idx >= _.bit_len) {
            return null;
        }
        let w_idx: usize = from_idx >> 6;
        const bit_off: u6 = @intCast(from_idx & 63);
        let w: u64 = _.words[w_idx] & (@as(u64, 0xFFFFFFFFFFFFFFFF) << bit_off);

        while (true) {
            if (w != 0) {
                const tz: usize = @as(usize, @ctz(w));
                const res: usize = (w_idx << 6) + tz;
                if (res >= _.bit_len) {
                    return null;
                }
                return res;
            }
            w_idx = w_idx + 1;
            if (w_idx >= _.words.len) {
                return null;
            }
            w = _.words[w_idx];
        }
    }

    pub fn unionWith(*_, other: BitSet) -> !void {
        if (_.bit_len != other.bit_len) {
            return BitSetError.SizeMismatch;
        }
        let i: usize = 0;
        while (i < _.words.len) {
            _.words[i] = _.words[i] | other.words[i];
            i = i + 1;
        }
    }

    pub fn intersectWith(*_, other: BitSet) -> !void {
        if (_.bit_len != other.bit_len) {
            return BitSetError.SizeMismatch;
        }
        let i: usize = 0;
        while (i < _.words.len) {
            _.words[i] = _.words[i] & other.words[i];
            i = i + 1;
        }
    }

    pub fn differenceWith(*_, other: BitSet) -> !void {
        if (_.bit_len != other.bit_len) {
            return BitSetError.SizeMismatch;
        }
        let i: usize = 0;
        while (i < _.words.len) {
            _.words[i] = _.words[i] & ~other.words[i];
            i = i + 1;
        }
    }

    pub fn xorWith(*_, other: BitSet) -> !void {
        if (_.bit_len != other.bit_len) {
            return BitSetError.SizeMismatch;
        }
        let i: usize = 0;
        while (i < _.words.len) {
            _.words[i] = _.words[i] ^ other.words[i];
            i = i + 1;
        }
    }

    pub fn equals(other: BitSet) -> bool {
        if (_.bit_len != other.bit_len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.words.len) {
            if (_.words[i] != other.words[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn bitLen() -> usize {
        return _.bit_len;
    }
}
