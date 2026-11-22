// Reference Only: Functional overlap with lib/ds/bitset.cp | Kept for comparison/analysis
// Source: cp/lib/bitset/bitset.cp (64-bit Word-Level Dynamic BitSet & Stack FixedBitSet)
// Pure CP zero-C-dependency bitset engine with hardware @popCount/@ctz acceleration,
// set algebra operations, range manipulation, and compact hex serialization.

const std = @import("std");

/// Error set for BitSet operations.
pub err BitSetError {
    InvalidRange,
    InvalidHex,
    OutOfMemory,
}

const HEX_LOWER: string = "0123456789abcdef";

fn hexNibble(ch: u8): u8? {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return null;
}

/// Dynamic 64-bit word-level bitset with automatic growth and hardware bit-scan acceleration.
pub struct BitSet {
    alloc: std.mem.Allocator,
    words: []u64 = [],

    /// Initialize an empty BitSet.
    pub fn init(alloc: std.mem.Allocator): BitSet {
        return BitSet{
            alloc: alloc,
            words: [],
        };
    }

    /// Initialize a BitSet pre-allocated for at least `bit_capacity` bits.
    pub fn initWithCapacity(alloc: std.mem.Allocator, bit_capacity: usize): BitSet ! BitSetError {
        if (bit_capacity == 0) {
            return BitSet.init(alloc);
        }
        const word_count = (bit_capacity + 63) / 64;
        const words = alloc.alloc(u64, word_count) catch return BitSetError.OutOfMemory;
        @memset(words, 0);
        return BitSet{
            alloc: alloc,
            words: words,
        };
    }

    /// Release backing word slice.
    pub fn deinit(*_) {
        if (_.words.len > 0) {
            _.alloc.free(_.words);
            _.words = [];
        }
    }

    /// Deep-copy this BitSet using `alloc`.
    pub fn clone(alloc: std.mem.Allocator): BitSet ! BitSetError {
        const active = _.activeWordCount();
        if (active == 0) {
            return BitSet.init(alloc);
        }
        const new_words = alloc.alloc(u64, active) catch return BitSetError.OutOfMemory;
        @memcpy(new_words, _.words[0..active]);
        return BitSet{
            alloc: alloc,
            words: new_words,
        };
    }

    fn ensureWords(*_, needed: usize): void ! BitSetError {
        if (needed <= _.words.len) return;
        let new_cap = if (_.words.len == 0) 4 else _.words.len * 2;
        if (new_cap < needed) {
            new_cap = needed;
        }
        const new_words = _.alloc.alloc(u64, new_cap) catch return BitSetError.OutOfMemory;
        if (_.words.len > 0) {
            @memcpy(new_words[0.._.words.len], _.words);
            @memset(new_words[_.words.len..new_cap], 0);
            _.alloc.free(_.words);
        } else {
            @memset(new_words, 0);
        }
        _.words = new_words;
    }

    fn activeWordCount(): usize {
        let i = _.words.len;
        while (i > 0) {
            i -= 1;
            if (_.words[i] != 0) {
                return i + 1;
            }
        }
        return 0;
    }

    /// Set the bit at `bit` to 1, growing backing storage if needed.
    pub fn set(*_, bit: usize): void ! BitSetError {
        const word_idx = bit / 64;
        const bit_off = u6(bit & 63);
        _.ensureWords(word_idx + 1)?;
        _.words[word_idx] |= (@as(u64, 1) << bit_off);
    }

    /// Clear the bit at `bit` to 0.
    pub fn clear(*_, bit: usize) {
        const word_idx = bit / 64;
        if (word_idx >= _.words.len) return;
        const bit_off = u6(bit & 63);
        _.words[word_idx] &= ~(@as(u64, 1) << bit_off);
    }

    /// Flip the bit at `bit`, growing backing storage if needed.
    pub fn toggle(*_, bit: usize): void ! BitSetError {
        const word_idx = bit / 64;
        const bit_off = u6(bit & 63);
        _.ensureWords(word_idx + 1)?;
        _.words[word_idx] ^= (@as(u64, 1) << bit_off);
    }

    /// Return true if `bit` is set.
    pub fn testBit(bit: usize): bool {
        const word_idx = bit / 64;
        if (word_idx >= _.words.len) return false;
        const bit_off = u6(bit & 63);
        return (_.words[word_idx] & (@as(u64, 1) << bit_off)) != 0;
    }

    /// Set all bits in half-open range [start_bit, end_bit).
    pub fn setRange(*_, start_bit: usize, end_bit: usize): void ! BitSetError {
        if (start_bit > end_bit) return BitSetError.InvalidRange;
        if (start_bit == end_bit) return;

        const last_bit = end_bit - 1;
        const start_word = start_bit / 64;
        const end_word = last_bit / 64;
        const start_off = u6(start_bit & 63);
        const end_off = u6(last_bit & 63);

        _.ensureWords(end_word + 1)?;

        const low_mask: u64 = (~@as(u64, 0)) << start_off;
        const high_mask: u64 = (~@as(u64, 0)) >> u6(63 - end_off);

        if (start_word == end_word) {
            _.words[start_word] |= (low_mask & high_mask);
            return;
        }

        _.words[start_word] |= low_mask;
        let w = start_word + 1;
        while (w < end_word) : (w += 1) {
            _.words[w] = ~@as(u64, 0);
        }
        _.words[end_word] |= high_mask;
    }

    /// Clear all bits in half-open range [start_bit, end_bit).
    pub fn clearRange(*_, start_bit: usize, end_bit: usize): void ! BitSetError {
        if (start_bit > end_bit) return BitSetError.InvalidRange;
        if (start_bit == end_bit) return;
        if (_.words.len == 0) return;

        const max_bits = _.words.len * 64;
        if (start_bit >= max_bits) return;

        const clipped_end = if (end_bit > max_bits) max_bits else end_bit;
        const last_bit = clipped_end - 1;
        const start_word = start_bit / 64;
        const end_word = last_bit / 64;
        const start_off = u6(start_bit & 63);
        const end_off = u6(last_bit & 63);

        const low_mask: u64 = (~@as(u64, 0)) << start_off;
        const high_mask: u64 = (~@as(u64, 0)) >> u6(63 - end_off);

        if (start_word == end_word) {
            _.words[start_word] &= ~(low_mask & high_mask);
            return;
        }

        _.words[start_word] &= ~low_mask;
        let w = start_word + 1;
        while (w < end_word) : (w += 1) {
            _.words[w] = 0;
        }
        _.words[end_word] &= ~high_mask;
    }

    /// Clear all bits without freeing backing capacity.
    pub fn clearAll(*_) {
        if (_.words.len > 0) {
            @memset(_.words, 0);
        }
    }

    /// Total number of set bits (population count) via hardware @popCount.
    pub fn count(): usize {
        let total: usize = 0;
        for (w of _.words) {
            total += (@popCount(w) as usize);
        }
        return total;
    }

    /// Return true if no bits are set.
    pub fn isEmpty(): bool {
        for (w of _.words) {
            if (w != 0) return false;
        }
        return true;
    }

    /// Return the lowest set bit index, or null if empty.
    pub fn firstSetBit(): usize? {
        return _.nextSetBit(0);
    }

    /// Return the lowest set bit index >= `from_bit`, or null if none.
    pub fn nextSetBit(from_bit: usize): usize? {
        let word_idx = from_bit / 64;
        if (word_idx >= _.words.len) return null;

        const bit_off = u6(from_bit & 63);
        const first_w = _.words[word_idx] & ((~@as(u64, 0)) << bit_off);
        if (first_w != 0) {
            return word_idx * 64 + (@ctz(first_w) as usize);
        }

        word_idx += 1;
        while (word_idx < _.words.len) : (word_idx += 1) {
            const w = _.words[word_idx];
            if (w != 0) {
                return word_idx * 64 + (@ctz(w) as usize);
            }
        }
        return null;
    }

    /// Collect all set bit indices in ascending order using @ctz zero-skipping.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn toIndices(alloc: std.mem.Allocator): []usize ! BitSetError {
        const total = _.count();
        let out = alloc.alloc(usize, total) catch return BitSetError.OutOfMemory;
        if (total == 0) return out;

        let pos: usize = 0;
        let word_idx: usize = 0;
        while (word_idx < _.words.len) : (word_idx += 1) {
            let w = _.words[word_idx];
            const base = word_idx * 64;
            while (w != 0) {
                const tz = (@ctz(w) as usize);
                out[pos] = base + tz;
                pos += 1;
                w &= w - 1;
            }
        }
        return out;
    }

    /// In-place union: self = self | other.
    pub fn unionWith(*_, other: BitSet): void ! BitSetError {
        const other_active = other.activeWordCount();
        if (other_active == 0) return;
        _.ensureWords(other_active)?;
        let i: usize = 0;
        while (i < other_active) : (i += 1) {
            _.words[i] |= other.words[i];
        }
    }

    /// In-place intersection: self = self & other.
    pub fn intersectWith(*_, other: BitSet) {
        const min_len = if (_.words.len < other.words.len) _.words.len else other.words.len;
        let i: usize = 0;
        while (i < min_len) : (i += 1) {
            _.words[i] &= other.words[i];
        }
        while (i < _.words.len) : (i += 1) {
            _.words[i] = 0;
        }
    }

    /// In-place set difference: self = self & ~other.
    pub fn differenceWith(*_, other: BitSet) {
        const min_len = if (_.words.len < other.words.len) _.words.len else other.words.len;
        let i: usize = 0;
        while (i < min_len) : (i += 1) {
            _.words[i] &= ~other.words[i];
        }
    }

    /// In-place symmetric difference: self = self ^ other.
    pub fn xorWith(*_, other: BitSet): void ! BitSetError {
        const other_active = other.activeWordCount();
        if (other_active == 0) return;
        _.ensureWords(other_active)?;
        let i: usize = 0;
        while (i < other_active) : (i += 1) {
            _.words[i] ^= other.words[i];
        }
    }

    /// Return true if self and other share at least one set bit.
    pub fn intersects(other: BitSet): bool {
        const min_len = if (_.words.len < other.words.len) _.words.len else other.words.len;
        let i: usize = 0;
        while (i < min_len) : (i += 1) {
            if ((_.words[i] & other.words[i]) != 0) return true;
        }
        return false;
    }

    /// Return true if every set bit in self is also set in other.
    pub fn isSubsetOf(other: BitSet): bool {
        const min_len = if (_.words.len < other.words.len) _.words.len else other.words.len;
        let i: usize = 0;
        while (i < min_len) : (i += 1) {
            if ((_.words[i] & ~other.words[i]) != 0) return false;
        }
        while (i < _.words.len) : (i += 1) {
            if (_.words[i] != 0) return false;
        }
        return true;
    }

    /// Return true if self and other have identical set bits (independent of backing capacity).
    pub fn equals(other: BitSet): bool {
        const min_len = if (_.words.len < other.words.len) _.words.len else other.words.len;
        let i: usize = 0;
        while (i < min_len) : (i += 1) {
            if (_.words[i] != other.words[i]) return false;
        }
        while (i < _.words.len) : (i += 1) {
            if (_.words[i] != 0) return false;
        }
        while (i < other.words.len) : (i += 1) {
            if (other.words[i] != 0) return false;
        }
        return true;
    }

    /// Compute Jaccard similarity |A ∩ B| / |A ∪ B|. Returns 1.0 if both sets are empty.
    pub fn jaccardSimilarity(other: BitSet): f64 {
        const max_len = if (_.words.len > other.words.len) _.words.len else other.words.len;
        let inter_count: usize = 0;
        let union_count: usize = 0;
        let i: usize = 0;
        while (i < max_len) : (i += 1) {
            const wa: u64 = if (i < _.words.len) _.words[i] else 0;
            const wb: u64 = if (i < other.words.len) other.words[i] else 0;
            inter_count += (@popCount(wa & wb) as usize);
            union_count += (@popCount(wa | wb) as usize);
        }
        if (union_count == 0) return 1.0;
        return (inter_count as f64) / (union_count as f64);
    }

    /// Serialize to compact lowercase hex string (little-endian byte order, trailing zero bytes trimmed).
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn toHex(alloc: std.mem.Allocator): []u8 ! BitSetError {
        const active = _.activeWordCount();
        if (active == 0) {
            return alloc.alloc(u8, 0) catch return BitSetError.OutOfMemory;
        }
        const top_word = _.words[active - 1];
        let top_bytes: usize = 8;
        while (top_bytes > 0) {
            const shift = u6((top_bytes - 1) * 8);
            if (((top_word >> shift) & 0xFF) != 0) break;
            top_bytes -= 1;
        }
        const total_bytes = (active - 1) * 8 + top_bytes;
        let out = alloc.alloc(u8, total_bytes * 2) catch return BitSetError.OutOfMemory;

        let b_idx: usize = 0;
        while (b_idx < total_bytes) : (b_idx += 1) {
            const w = _.words[b_idx / 8];
            const shift = u6((b_idx & 7) * 8);
            const byte_val = u8((w >> shift) & 0xFF);
            out[b_idx * 2] = HEX_LOWER[byte_val >> 4];
            out[b_idx * 2 + 1] = HEX_LOWER[byte_val & 0x0F];
        }
        return out;
    }

    /// Deserialize from compact hex string produced by `toHex`.
    pub fn fromHex(alloc: std.mem.Allocator, hex_str: string): BitSet ! BitSetError {
        if ((hex_str.len & 1) != 0) return BitSetError.InvalidHex;
        if (hex_str.len == 0) return BitSet.init(alloc);

        const num_bytes = hex_str.len / 2;
        const num_words = (num_bytes + 7) / 8;

        // Validate all characters before allocating.
        let k: usize = 0;
        while (k < hex_str.len) : (k += 1) {
            if (hexNibble(hex_str[k]) == null) {
                return BitSetError.InvalidHex;
            }
        }

        let words = alloc.alloc(u64, num_words) catch return BitSetError.OutOfMemory;
        @memset(words, 0);

        let b_idx: usize = 0;
        while (b_idx < num_bytes) : (b_idx += 1) {
            const hi = hexNibble(hex_str[b_idx * 2]).?;
            const lo = hexNibble(hex_str[b_idx * 2 + 1]).?;
            const byte_val: u64 = (u64(hi) << 4) | u64(lo);
            const shift = u6((b_idx & 7) * 8);
            words[b_idx / 8] |= (byte_val << shift);
        }

        return BitSet{
            alloc: alloc,
            words: words,
        };
    }
}

/// Zero-heap stack-allocated fixed-size N-bit bitset.
pub fn FixedBitSet(comptime N: usize): type {
    const WORD_COUNT: usize = (N + 63) / 64;
    return struct {
        const Self = @This();

        words: [WORD_COUNT]u64 = [0; WORD_COUNT],

        /// Initialize a zeroed FixedBitSet on the stack.
        pub fn init(): Self {
            return Self{
                words: [0; WORD_COUNT],
            };
        }

        /// Set bit `bit` (ignored if bit >= N).
        pub fn set(*_, bit: usize) {
            if (bit >= N) return;
            const word_idx = bit / 64;
            const bit_off = u6(bit & 63);
            _.words[word_idx] |= (@as(u64, 1) << bit_off);
        }

        /// Clear bit `bit` (ignored if bit >= N).
        pub fn clear(*_, bit: usize) {
            if (bit >= N) return;
            const word_idx = bit / 64;
            const bit_off = u6(bit & 63);
            _.words[word_idx] &= ~(@as(u64, 1) << bit_off);
        }

        /// Flip bit `bit` (ignored if bit >= N).
        pub fn toggle(*_, bit: usize) {
            if (bit >= N) return;
            const word_idx = bit / 64;
            const bit_off = u6(bit & 63);
            _.words[word_idx] ^= (@as(u64, 1) << bit_off);
        }

        /// Return true if bit `bit` is set (false if bit >= N).
        pub fn testBit(bit: usize): bool {
            if (bit >= N) return false;
            const word_idx = bit / 64;
            const bit_off = u6(bit & 63);
            return (_.words[word_idx] & (@as(u64, 1) << bit_off)) != 0;
        }

        /// Clear all bits.
        pub fn clearAll(*_) {
            _.words = [0; WORD_COUNT];
        }

        /// Total number of set bits via hardware @popCount.
        pub fn count(): usize {
            let total: usize = 0;
            for (w of _.words) {
                total += (@popCount(w) as usize);
            }
            return total;
        }

        /// Return true if no bits are set.
        pub fn isEmpty(): bool {
            for (w of _.words) {
                if (w != 0) return false;
            }
            return true;
        }

        /// In-place union with another FixedBitSet(N).
        pub fn unionWith(*_, other: Self) {
            let i: usize = 0;
            while (i < WORD_COUNT) : (i += 1) {
                _.words[i] |= other.words[i];
            }
        }

        /// In-place intersection with another FixedBitSet(N).
        pub fn intersectWith(*_, other: Self) {
            let i: usize = 0;
            while (i < WORD_COUNT) : (i += 1) {
                _.words[i] &= other.words[i];
            }
        }

        /// Return true if self and other have identical set bits.
        pub fn equals(other: Self): bool {
            let i: usize = 0;
            while (i < WORD_COUNT) : (i += 1) {
                if (_.words[i] != other.words[i]) return false;
            }
            return true;
        }
    };
}
