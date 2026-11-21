// Source: /data/dev/bun/src/collections/bit_set.cp
// ============================================================================
// Absorbed from Bun: bun/src/collections/bit_set.cp
// Upstream Sync: Refer to bun/src/collections/bit_set.zig for future upstream updates.
// ============================================================================
// lib/col/bit_set.cp
// Industrial-grade BitSet suite: Range, BitSet64, BitSet128, BitSet256, BitSet512, BitSet1024, DynamicBitSet
// Absorbed from Bun src/collections/bit_set.cp

pub struct Range {
    pub start: usize = 0;
    pub end: usize = 0;

    pub static fn init(start: usize, end: usize): Range {
        return Range { start: start, end: end };
    }
}

pub struct BitSet64Iter {
    pub mask: u64 = 0;

    pub static fn init(mask: u64): BitSet64Iter {
        return BitSet64Iter { mask: mask };
    }

    pub fn next(*_): usize? {
        if (_.mask == 0) {
            return null;
        }
        const idx = @ctz(_.mask) as usize;
        _.mask &= _.mask - 1;
        return idx;
    }
}

pub struct BitSet64 {
    pub mask: u64 = 0;

    pub static fn initEmpty(): BitSet64 {
        return BitSet64 { mask: 0 };
    }

    pub static fn initFull(): BitSet64 {
        return BitSet64 { mask: 0xFFFFFFFFFFFFFFFF };
    }

    pub fn capacity(): usize {
        return if (_.mask == 0) 64 else 64;
    }

    pub fn count(): usize {
        return @popCount(_.mask) as usize;
    }

    pub fn isSet(index: usize): bool {
        if (index >= 64) {
            return false;
        }
        const bit: u64 = (1 as u64) << index;
        return (_.mask & bit) != 0;
    }

    pub fn set(*_, index: usize) {
        if (index < 64) {
            const bit: u64 = (1 as u64) << index;
            _.mask |= bit;
        }
    }

    pub fn unset(*_, index: usize) {
        if (index < 64) {
            const bit: u64 = (1 as u64) << index;
            _.mask &= ~bit;
        }
    }

    pub fn setValue(*_, index: usize, value: bool) {
        if (value) {
            _.set(index);
        } else {
            _.unset(index);
        }
    }

    pub fn toggle(*_, index: usize) {
        if (index < 64) {
            const bit: u64 = (1 as u64) << index;
            _.mask ^= bit;
        }
    }

    pub fn toggleSet(*_, other: BitSet64) {
        _.mask ^= other.mask;
    }

    pub fn toggleAll(*_) {
        _.mask = ~_.mask;
    }

    pub fn setAll(*_, value: bool) {
        _.mask = if (value) 0xFFFFFFFFFFFFFFFF else 0;
    }

    pub fn setRangeValue(*_, range: Range, value: bool) {
        if (range.start >= range.end || range.start >= 64) {
            return;
        }
        const end_idx = if (range.end < 64) range.end else 64;
        let i = range.start;
        while (i < end_idx) {
            const bit: u64 = (1 as u64) << i;
            if (value) {
                _.mask |= bit;
            } else {
                _.mask &= ~bit;
            }
            i += 1;
        }
    }

    pub fn setUnion(*_, other: BitSet64) {
        _.mask |= other.mask;
    }

    pub fn setIntersection(*_, other: BitSet64) {
        _.mask &= other.mask;
    }

    pub fn unionWith(other: BitSet64): BitSet64 {
        return BitSet64 { mask: _.mask | other.mask };
    }

    pub fn intersectWith(other: BitSet64): BitSet64 {
        return BitSet64 { mask: _.mask & other.mask };
    }

    pub fn xorWith(other: BitSet64): BitSet64 {
        return BitSet64 { mask: _.mask ^ other.mask };
    }

    pub fn differenceWith(other: BitSet64): BitSet64 {
        return BitSet64 { mask: _.mask & ~other.mask };
    }

    pub fn complement(): BitSet64 {
        return BitSet64 { mask: ~_.mask };
    }

    pub fn eql(other: BitSet64): bool {
        return _.mask == other.mask;
    }

    pub fn subsetOf(other: BitSet64): bool {
        return (_.mask & other.mask) == _.mask;
    }

    pub fn supersetOf(other: BitSet64): bool {
        return (_.mask & other.mask) == other.mask;
    }

    pub fn hasIntersection(other: BitSet64): bool {
        return (_.mask & other.mask) != 0;
    }

    pub fn findFirstSet(limit: usize): usize? {
        if (_.mask == 0) {
            return null;
        }
        const idx = @ctz(_.mask) as usize;
        if (idx >= limit) {
            return null;
        }
        return idx;
    }

    pub fn findFirstUnset(limit: usize): usize? {
        const inverted: u64 = ~_.mask;
        if (inverted == 0) {
            return null;
        }
        const idx = @ctz(inverted) as usize;
        if (idx >= limit) {
            return null;
        }
        return idx;
    }

    pub fn findFirst(): usize? {
        return _.findFirstSet(64);
    }

    pub fn toggleFirstSet(*_): usize? {
        if (idx := _.findFirstSet(64)) {
            _.unset(idx);
            return idx;
        }
        return null;
    }

    pub fn iterator(): BitSet64Iter {
        return BitSet64Iter.init(_.mask);
    }
}

pub struct BitSet128 {
    pub masks: [2]u64 = [0, 0];

    pub static fn initEmpty(): BitSet128 {
        return BitSet128 { masks: [0, 0] };
    }

    pub static fn initFull(): BitSet128 {
        return BitSet128 { masks: [0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF] };
    }

    pub fn capacity(): usize {
        return _.masks.len * 64;
    }

    pub fn count(): usize {
        return (@popCount(_.masks[0]) as usize) + (@popCount(_.masks[1]) as usize);
    }

    pub fn isSet(index: usize): bool {
        if (index >= 128) {
            return false;
        }
        const word_idx = index >> 6;
        const bit_idx = index & 63;
        const bit: u64 = (1 as u64) << bit_idx;
        return (_.masks[word_idx] & bit) != 0;
    }

    pub fn set(*_, index: usize) {
        if (index < 128) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] |= bit;
        }
    }

    pub fn unset(*_, index: usize) {
        if (index < 128) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] &= ~bit;
        }
    }

    pub fn setValue(*_, index: usize, value: bool) {
        if (value) {
            _.set(index);
        } else {
            _.unset(index);
        }
    }

    pub fn toggle(*_, index: usize) {
        if (index < 128) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] ^= bit;
        }
    }

    pub fn toggleAll(*_) {
        _.masks[0] = ~_.masks[0];
        _.masks[1] = ~_.masks[1];
    }

    pub fn setAll(*_, value: bool) {
        const m: u64 = if (value) 0xFFFFFFFFFFFFFFFF else 0;
        _.masks[0] = m;
        _.masks[1] = m;
    }

    pub fn setUnion(*_, other: BitSet128) {
        _.masks[0] |= other.masks[0];
        _.masks[1] |= other.masks[1];
    }

    pub fn setIntersection(*_, other: BitSet128) {
        _.masks[0] &= other.masks[0];
        _.masks[1] &= other.masks[1];
    }

    pub fn eql(other: BitSet128): bool {
        return _.masks[0] == other.masks[0] && _.masks[1] == other.masks[1];
    }

    pub fn findFirstSet(): usize? {
        let i: usize = 0;
        while (i < 2) {
            if (_.masks[i] != 0) {
                return (i << 6) + (@ctz(_.masks[i]) as usize);
            }
            i += 1;
        }
        return null;
    }

    pub fn findFirstUnset(): usize? {
        let i: usize = 0;
        while (i < 2) {
            const inv = ~_.masks[i];
            if (inv != 0) {
                return (i << 6) + (@ctz(inv) as usize);
            }
            i += 1;
        }
        return null;
    }
}

pub struct BitSet256Iter {
    pub masks: [4]u64 = [0, 0, 0, 0];
    pub word_idx: usize = 0;
    pub current_word: u64 = 0;

    pub static fn init(masks: [4]u64): BitSet256Iter {
        return BitSet256Iter {
            masks: masks,
            word_idx: 0,
            current_word: masks[0],
        };
    }

    pub fn next(*_): usize? {
        while (_.current_word == 0) {
            _.word_idx += 1;
            if (_.word_idx >= 4) {
                return null;
            }
            _.current_word = _.masks[_.word_idx];
        }
        const bit = @ctz(_.current_word) as usize;
        _.current_word &= _.current_word - 1;
        return (_.word_idx << 6) + bit;
    }
}

pub struct BitSet256 {
    pub masks: [4]u64 = [0, 0, 0, 0];

    pub static fn initEmpty(): BitSet256 {
        return BitSet256 {
            masks: [0, 0, 0, 0],
        };
    }

    pub static fn initFull(): BitSet256 {
        return BitSet256 {
            masks: [
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
            ],
        };
    }

    pub fn capacity(): usize {
        return _.masks.len * 64;
    }

    pub fn count(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < 4) {
            total += (@popCount(_.masks[i]) as usize);
            i += 1;
        }
        return total;
    }

    pub fn isSet(index: usize): bool {
        if (index >= 256) {
            return false;
        }
        const word_idx = index >> 6;
        const bit_idx = index & 63;
        const bit: u64 = (1 as u64) << bit_idx;
        return (_.masks[word_idx] & bit) != 0;
    }

    pub fn set(*_, index: usize) {
        if (index < 256) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] |= bit;
        }
    }

    pub fn unset(*_, index: usize) {
        if (index < 256) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] &= ~bit;
        }
    }

    pub fn setValue(*_, index: usize, value: bool) {
        if (value) {
            _.set(index);
        } else {
            _.unset(index);
        }
    }

    pub fn toggle(*_, index: usize) {
        if (index < 256) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] ^= bit;
        }
    }

    pub fn toggleSet(*_, other: BitSet256) {
        let i: usize = 0;
        while (i < 4) {
            _.masks[i] ^= other.masks[i];
            i += 1;
        }
    }

    pub fn toggleAll(*_) {
        let i: usize = 0;
        while (i < 4) {
            _.masks[i] = ~_.masks[i];
            i += 1;
        }
    }

    pub fn setAll(*_, value: bool) {
        const m: u64 = if (value) 0xFFFFFFFFFFFFFFFF else 0;
        let i: usize = 0;
        while (i < 4) {
            _.masks[i] = m;
            i += 1;
        }
    }

    pub fn setRangeValue(*_, range: Range, value: bool) {
        if (range.start >= range.end || range.start >= 256) {
            return;
        }
        const full_mask: u64 = if (value) (0xFFFFFFFFFFFFFFFF as u64) else 0;
        const end_idx = if (range.end < 256) range.end else 256;
        let i = range.start;
        while (i < end_idx) {
            const bit_offset = i & 63;
            const word_idx = i >> 6;
            if (bit_offset == 0 && (i + 64) <= end_idx) {
                _.masks[word_idx] = full_mask;
                i += 64;
            } else {
                const bit: u64 = (1 as u64) << bit_offset;
                if (value) {
                    _.masks[word_idx] |= bit;
                } else {
                    _.masks[word_idx] &= ~bit;
                }
                i += 1;
            }
        }
    }

    pub fn findFirstSet(): usize? {
        let i: usize = 0;
        while (i < 4) {
            if (_.masks[i] != 0) {
                return (i << 6) + (@ctz(_.masks[i]) as usize);
            }
            i += 1;
        }
        return null;
    }

    pub fn findFirstUnset(): usize? {
        let i: usize = 0;
        while (i < 4) {
            const inv = ~_.masks[i];
            if (inv != 0) {
                return (i << 6) + (@ctz(inv) as usize);
            }
            i += 1;
        }
        return null;
    }

    pub fn toggleFirstSet(*_): usize? {
        if (idx := _.findFirstSet()) {
            _.unset(idx);
            return idx;
        }
        return null;
    }

    pub fn setUnion(*_, other: BitSet256) {
        let i: usize = 0;
        while (i < 4) {
            _.masks[i] |= other.masks[i];
            i += 1;
        }
    }

    pub fn setIntersection(*_, other: BitSet256) {
        let i: usize = 0;
        while (i < 4) {
            _.masks[i] &= other.masks[i];
            i += 1;
        }
    }

    pub fn unionWith(other: BitSet256): BitSet256 {
        return BitSet256 {
            masks: [
                _.masks[0] | other.masks[0],
                _.masks[1] | other.masks[1],
                _.masks[2] | other.masks[2],
                _.masks[3] | other.masks[3],
            ],
        };
    }

    pub fn intersectWith(other: BitSet256): BitSet256 {
        return BitSet256 {
            masks: [
                _.masks[0] & other.masks[0],
                _.masks[1] & other.masks[1],
                _.masks[2] & other.masks[2],
                _.masks[3] & other.masks[3],
            ],
        };
    }

    pub fn xorWith(other: BitSet256): BitSet256 {
        return BitSet256 {
            masks: [
                _.masks[0] ^ other.masks[0],
                _.masks[1] ^ other.masks[1],
                _.masks[2] ^ other.masks[2],
                _.masks[3] ^ other.masks[3],
            ],
        };
    }

    pub fn differenceWith(other: BitSet256): BitSet256 {
        return BitSet256 {
            masks: [
                _.masks[0] & ~other.masks[0],
                _.masks[1] & ~other.masks[1],
                _.masks[2] & ~other.masks[2],
                _.masks[3] & ~other.masks[3],
            ],
        };
    }

    pub fn complement(): BitSet256 {
        return BitSet256 {
            masks: [
                ~_.masks[0],
                ~_.masks[1],
                ~_.masks[2],
                ~_.masks[3],
            ],
        };
    }

    pub fn eql(other: BitSet256): bool {
        let i: usize = 0;
        while (i < 4) {
            if (_.masks[i] != other.masks[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn subsetOf(other: BitSet256): bool {
        let i: usize = 0;
        while (i < 4) {
            if ((_.masks[i] & other.masks[i]) != _.masks[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn supersetOf(other: BitSet256): bool {
        return other.subsetOf(_);
    }

    pub fn hasIntersection(other: BitSet256): bool {
        let i: usize = 0;
        while (i < 4) {
            if ((_.masks[i] & other.masks[i]) != 0) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn iterator(): BitSet256Iter {
        return BitSet256Iter.init(_.masks);
    }
}

pub struct BitSet512 {
    pub masks: [8]u64 = [0, 0, 0, 0, 0, 0, 0, 0];

    pub static fn initEmpty(): BitSet512 {
        return BitSet512 {
            masks: [0, 0, 0, 0, 0, 0, 0, 0],
        };
    }

    pub static fn initFull(): BitSet512 {
        return BitSet512 {
            masks: [
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
            ],
        };
    }

    pub fn capacity(): usize {
        return _.masks.len * 64;
    }

    pub fn count(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < 8) {
            total += (@popCount(_.masks[i]) as usize);
            i += 1;
        }
        return total;
    }

    pub fn isSet(index: usize): bool {
        if (index >= 512) {
            return false;
        }
        const word_idx = index >> 6;
        const bit_idx = index & 63;
        const bit: u64 = (1 as u64) << bit_idx;
        return (_.masks[word_idx] & bit) != 0;
    }

    pub fn set(*_, index: usize) {
        if (index < 512) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] |= bit;
        }
    }

    pub fn unset(*_, index: usize) {
        if (index < 512) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] &= ~bit;
        }
    }

    pub fn setValue(*_, index: usize, value: bool) {
        if (value) {
            _.set(index);
        } else {
            _.unset(index);
        }
    }

    pub fn toggle(*_, index: usize) {
        if (index < 512) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] ^= bit;
        }
    }

    pub fn toggleAll(*_) {
        let i: usize = 0;
        while (i < 8) {
            _.masks[i] = ~_.masks[i];
            i += 1;
        }
    }

    pub fn setAll(*_, value: bool) {
        const m: u64 = if (value) 0xFFFFFFFFFFFFFFFF else 0;
        let i: usize = 0;
        while (i < 8) {
            _.masks[i] = m;
            i += 1;
        }
    }

    pub fn setUnion(*_, other: BitSet512) {
        let i: usize = 0;
        while (i < 8) {
            _.masks[i] |= other.masks[i];
            i += 1;
        }
    }

    pub fn setIntersection(*_, other: BitSet512) {
        let i: usize = 0;
        while (i < 8) {
            _.masks[i] &= other.masks[i];
            i += 1;
        }
    }

    pub fn eql(other: BitSet512): bool {
        let i: usize = 0;
        while (i < 8) {
            if (_.masks[i] != other.masks[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn findFirstSet(): usize? {
        let i: usize = 0;
        while (i < 8) {
            if (_.masks[i] != 0) {
                return (i << 6) + (@ctz(_.masks[i]) as usize);
            }
            i += 1;
        }
        return null;
    }
}

pub struct BitSet1024 {
    pub masks: [16]u64 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

    pub static fn initEmpty(): BitSet1024 {
        return BitSet1024 {
            masks: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        };
    }

    pub static fn initFull(): BitSet1024 {
        return BitSet1024 {
            masks: [
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
            ],
        };
    }

    pub fn capacity(): usize {
        return _.masks.len * 64;
    }

    pub fn count(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < 16) {
            total += (@popCount(_.masks[i]) as usize);
            i += 1;
        }
        return total;
    }

    pub fn isSet(index: usize): bool {
        if (index >= 1024) {
            return false;
        }
        const word_idx = index >> 6;
        const bit_idx = index & 63;
        const bit: u64 = (1 as u64) << bit_idx;
        return (_.masks[word_idx] & bit) != 0;
    }

    pub fn set(*_, index: usize) {
        if (index < 1024) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] |= bit;
        }
    }

    pub fn unset(*_, index: usize) {
        if (index < 1024) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] &= ~bit;
        }
    }

    pub fn setValue(*_, index: usize, value: bool) {
        if (value) {
            _.set(index);
        } else {
            _.unset(index);
        }
    }

    pub fn toggle(*_, index: usize) {
        if (index < 1024) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] ^= bit;
        }
    }

    pub fn toggleAll(*_) {
        let i: usize = 0;
        while (i < 16) {
            _.masks[i] = ~_.masks[i];
            i += 1;
        }
    }

    pub fn setAll(*_, value: bool) {
        const m: u64 = if (value) 0xFFFFFFFFFFFFFFFF else 0;
        let i: usize = 0;
        while (i < 16) {
            _.masks[i] = m;
            i += 1;
        }
    }

    pub fn setUnion(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            _.masks[i] |= other.masks[i];
            i += 1;
        }
    }

    pub fn setIntersection(*_, other: BitSet1024) {
        let i: usize = 0;
        while (i < 16) {
            _.masks[i] &= other.masks[i];
            i += 1;
        }
    }

    pub fn eql(other: BitSet1024): bool {
        let i: usize = 0;
        while (i < 16) {
            if (_.masks[i] != other.masks[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn findFirstSet(): usize? {
        let i: usize = 0;
        while (i < 16) {
            if (_.masks[i] != 0) {
                return (i << 6) + (@ctz(_.masks[i]) as usize);
            }
            i += 1;
        }
        return null;
    }
}

pub class DynamicBitSet {
    pub masks: []u64 = [];
    pub bit_length: usize = 0;
    pub cap_words: usize = 0;

    pub static fn initEmpty(bit_len: usize): *DynamicBitSet {
        const num_words = (bit_len + 63) / 64;
        const alloc_words = if (num_words > 0) num_words else 1;
        let bs = std.heap.page_allocator.create(@This()) catch unreachable;
        let m = std.heap.page_allocator.alloc(u64, alloc_words) catch unreachable;
        let i: usize = 0;
        while (i < alloc_words) {
            m[i] = 0;
            i += 1;
        }
        bs.masks = m;
        bs.bit_length = bit_len;
        bs.cap_words = alloc_words;
        return bs;
    }

    pub static fn initFull(bit_len: usize): *DynamicBitSet {
        let bs = DynamicBitSet.initEmpty(bit_len);
        bs.setAll(true);
        return bs;
    }

    pub fn capacity(*_): usize {
        return _.bit_length;
    }

    pub fn count(*_): usize {
        let total: usize = 0;
        const num_words = (_.bit_length + 63) / 64;
        let i: usize = 0;
        while (i < num_words) {
            total += (@popCount(_.masks[i]) as usize);
            i += 1;
        }
        return total;
    }

    pub fn isSet(*_, index: usize): bool {
        if (index >= _.bit_length) {
            return false;
        }
        const word_idx = index >> 6;
        const bit_idx = index & 63;
        const bit: u64 = (1 as u64) << bit_idx;
        return (_.masks[word_idx] & bit) != 0;
    }

    pub fn set(*_, index: usize) {
        if (index < _.bit_length) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] |= bit;
        }
    }

    pub fn unset(*_, index: usize) {
        if (index < _.bit_length) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] &= ~bit;
        }
    }

    pub fn setValue(*_, index: usize, value: bool) {
        if (value) {
            _.set(index);
        } else {
            _.unset(index);
        }
    }

    pub fn toggle(*_, index: usize) {
        if (index < _.bit_length) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] ^= bit;
        }
    }

    pub fn setAll(*_, value: bool) {
        const fill_val: u64 = if (value) 0xFFFFFFFFFFFFFFFF else 0;
        const num_words = (_.bit_length + 63) / 64;
        let i: usize = 0;
        while (i < num_words) {
            _.masks[i] = fill_val;
            i += 1;
        }
        if (value && (_.bit_length & 63) != 0) {
            const pad_bits = 64 - (_.bit_length & 63);
            const last_mask: u64 = (0xFFFFFFFFFFFFFFFF as u64) >> pad_bits;
            _.masks[num_words - 1] &= last_mask;
        }
    }

    pub fn toggleAll(*_) {
        const num_words = (_.bit_length + 63) / 64;
        let i: usize = 0;
        while (i < num_words) {
            _.masks[i] = ~_.masks[i];
            i += 1;
        }
        if ((_.bit_length & 63) != 0) {
            const pad_bits = 64 - (_.bit_length & 63);
            const last_mask: u64 = (0xFFFFFFFFFFFFFFFF as u64) >> pad_bits;
            _.masks[num_words - 1] &= last_mask;
        }
    }

    pub fn setRangeValue(*_, range: Range, value: bool) {
        if (range.start >= range.end || range.start >= _.bit_length) {
            return;
        }
        const full_mask: u64 = if (value) (0xFFFFFFFFFFFFFFFF as u64) else 0;
        const end_idx = if (range.end < _.bit_length) range.end else _.bit_length;
        let i = range.start;
        while (i < end_idx) {
            const bit_offset = i & 63;
            const word_idx = i >> 6;
            if (bit_offset == 0 && (i + 64) <= end_idx) {
                _.masks[word_idx] = full_mask;
                i += 64;
            } else {
                const bit: u64 = (1 as u64) << bit_offset;
                if (value) {
                    _.masks[word_idx] |= bit;
                } else {
                    _.masks[word_idx] &= ~bit;
                }
                i += 1;
            }
        }
    }

    pub fn resize(*_, new_len: usize, fill: bool) {
        if (new_len == _.bit_length) {
            return;
        }
        const new_words = (new_len + 63) / 64;
        const old_words = (_.bit_length + 63) / 64;
        if (new_words > _.cap_words) {
            let new_cap_words = _.cap_words * 2;
            if (new_cap_words < new_words) {
                new_cap_words = new_words;
            }
            let new_m = std.heap.page_allocator.alloc(u64, new_cap_words) catch unreachable;
            let i: usize = 0;
            while (i < old_words) {
                new_m[i] = _.masks[i];
                i += 1;
            }
            while (i < new_cap_words) {
                new_m[i] = 0;
                i += 1;
            }
            if (_.cap_words > 0) {
                std.heap.page_allocator.free(_.masks);
            }
            _.masks = new_m;
            _.cap_words = new_cap_words;
        }
        const old_len = _.bit_length;
        _.bit_length = new_len;
        if (fill && new_len > old_len) {
            let r = Range { start: old_len, end: new_len };
            _.setRangeValue(r, true);
        } else if (new_len < old_len) {
            if ((new_len & 63) != 0) {
                const pad_bits = 64 - (new_len & 63);
                const last_mask: u64 = (0xFFFFFFFFFFFFFFFF as u64) >> pad_bits;
                _.masks[new_words - 1] &= last_mask;
            }
            let i = new_words;
            while (i < old_words) {
                _.masks[i] = 0;
                i += 1;
            }
        }
    }

    pub fn setUnion(*_, other: *DynamicBitSet) {
        const words = if ((_.bit_length + 63) / 64 < (other.bit_length + 63) / 64) ((_.bit_length + 63) / 64) else ((other.bit_length + 63) / 64);
        let i: usize = 0;
        while (i < words) {
            _.masks[i] |= other.masks[i];
            i += 1;
        }
    }

    pub fn setIntersection(*_, other: *DynamicBitSet) {
        const my_words = (_.bit_length + 63) / 64;
        const other_words = (other.bit_length + 63) / 64;
        const words = if (my_words < other_words) my_words else other_words;
        let i: usize = 0;
        while (i < words) {
            _.masks[i] &= other.masks[i];
            i += 1;
        }
        while (i < my_words) {
            _.masks[i] = 0;
            i += 1;
        }
    }

    pub fn setExclude(*_, other: *DynamicBitSet) {
        const words = if ((_.bit_length + 63) / 64 < (other.bit_length + 63) / 64) ((_.bit_length + 63) / 64) else ((other.bit_length + 63) / 64);
        let i: usize = 0;
        while (i < words) {
            _.masks[i] &= ~other.masks[i];
            i += 1;
        }
    }

    pub fn toggleSet(*_, other: *DynamicBitSet) {
        const words = if ((_.bit_length + 63) / 64 < (other.bit_length + 63) / 64) ((_.bit_length + 63) / 64) else ((other.bit_length + 63) / 64);
        let i: usize = 0;
        while (i < words) {
            _.masks[i] ^= other.masks[i];
            i += 1;
        }
    }

    pub fn hasIntersection(*_, other: *DynamicBitSet): bool {
        const words = if ((_.bit_length + 63) / 64 < (other.bit_length + 63) / 64) ((_.bit_length + 63) / 64) else ((other.bit_length + 63) / 64);
        let i: usize = 0;
        while (i < words) {
            if ((_.masks[i] & other.masks[i]) != 0) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn eql(*_, other: *DynamicBitSet): bool {
        if (_.bit_length != other.bit_length) {
            return false;
        }
        const words = (_.bit_length + 63) / 64;
        let i: usize = 0;
        while (i < words) {
            if (_.masks[i] != other.masks[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn subsetOf(*_, other: *DynamicBitSet): bool {
        const my_words = (_.bit_length + 63) / 64;
        const other_words = (other.bit_length + 63) / 64;
        let i: usize = 0;
        while (i < my_words) {
            const om = if (i < other_words) other.masks[i] else 0;
            if ((_.masks[i] & om) != _.masks[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn supersetOf(*_, other: *DynamicBitSet): bool {
        return other.subsetOf(_);
    }

    pub fn findFirstSet(*_): usize? {
        const words = (_.bit_length + 63) / 64;
        let i: usize = 0;
        while (i < words) {
            if (_.masks[i] != 0) {
                const idx = (i << 6) + (@ctz(_.masks[i]) as usize);
                if (idx < _.bit_length) {
                    return idx;
                }
            }
            i += 1;
        }
        return null;
    }

    pub fn findFirstUnset(*_): usize? {
        const words = (_.bit_length + 63) / 64;
        let i: usize = 0;
        while (i < words) {
            const inv = ~_.masks[i];
            if (inv != 0) {
                const idx = (i << 6) + (@ctz(inv) as usize);
                if (idx < _.bit_length) {
                    return idx;
                }
            }
            i += 1;
        }
        return null;
    }

    pub fn toggleFirstSet(*_): usize? {
        if (idx := _.findFirstSet()) {
            _.unset(idx);
            return idx;
        }
        return null;
    }

    pub fn clone(*_): *DynamicBitSet {
        let cloned = DynamicBitSet.initEmpty(_.bit_length);
        const words = (_.bit_length + 63) / 64;
        let i: usize = 0;
        while (i < words) {
            cloned.masks[i] = _.masks[i];
            i += 1;
        }
        return cloned;
    }

    pub fn deinit(*_) {
        if (_.cap_words > 0) {
            std.heap.page_allocator.free(_.masks);
            _.masks = [];
            _.cap_words = 0;
            _.bit_length = 0;
        }
    }

    ~init() {
        _.deinit();
        std.heap.page_allocator.destroy(_);
    }
}
