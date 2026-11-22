// Origin: Pure-CP Stdlib (Zero-Alloc LFU Cache & 4-bit W-TinyLFU Count-Min Sketch) | Created: 2026-11-22

/// Number of independent hash rows in `TinyLfuSketch`.
pub const TINYLFU_ROWS: usize = 4;

/// Maximum saturated value of each 4-bit counter in `TinyLfuSketch`.
pub const TINYLFU_MAX_COUNT: u8 = 15;

const ROW_SEEDS: [4]u64 = [
    0x9E3779B97F4A7C15,
    0xBF58476D1CE4E5B9,
    0x94D049BB133111EB,
    0xD6E8FEB86659FD93,
];

fn mulWrap64(a: u64, b: u64): u64 {
    return (((a as u128) * (b as u128)) & 0xFFFFFFFFFFFFFFFF) as u64;
}

fn hashRow(key: u64, row: usize): u64 {
    let x = key ^ ROW_SEEDS[row];
    x = mulWrap64(x ^ (x >> 30), 0xBF58476D1CE4E5B9);
    x = mulWrap64(x ^ (x >> 27), 0x94D049BB133111EB);
    return x ^ (x >> 31);
}

fn isPow2(n: usize): bool {
    return n > 0 && (n & (n - 1)) == 0;
}

/// Zero-allocation 4-row, 4-bit saturating Count-Min Sketch (W-TinyLFU admission filter).
/// Each byte in `counters` packs two 4-bit counters (`0..15`).
pub struct TinyLfuSketch {
    counters: []u8;
    row_width: usize;
    sample_size: u32;
    reset_threshold: u32;

    /// Initializes a `TinyLfuSketch` over caller-owned `counters`.
    /// Requires `counters.len >= 8`, power-of-two `counters.len`, and `reset_threshold > 0`.
    pub fn init(counters: []u8, reset_threshold: u32): TinyLfuSketch? {
        if (counters.len < 8 || !isPow2(counters.len) || reset_threshold == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < counters.len) {
            counters[i] = 0;
            i += 1;
        }
        return TinyLfuSketch {
            counters: counters,
            row_width: counters.len / 2,
            sample_size: 0,
            reset_threshold: reset_threshold,
        };
    }

    fn readNibble(row: usize, col: usize): u8 {
        const nibble_idx = row * _.row_width + col;
        const b = _.counters[nibble_idx >> 1];
        if ((nibble_idx & 1) == 0) {
            return b & 0x0F;
        }
        return (b >> 4) & 0x0F;
    }

    fn writeNibble(*_, row: usize, col: usize, val: u8) {
        const nibble_idx = row * _.row_width + col;
        const byte_idx = nibble_idx >> 1;
        const v = val & 0x0F;
        if ((nibble_idx & 1) == 0) {
            _.counters[byte_idx] = (_.counters[byte_idx] & 0xF0) | v;
        } else {
            _.counters[byte_idx] = (_.counters[byte_idx] & 0x0F) | (v << 4);
        }
    }

    /// Halves all 4-bit counters in-place (aging decay) and resets `sample_size` to 0.
    fn halve(*_) {
        let i: usize = 0;
        while (i < _.counters.len) {
            _.counters[i] = (_.counters[i] >> 1) & 0x77;
            i += 1;
        }
        _.sample_size = 0;
    }

    /// Clears all counters and resets `sample_size` to 0.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.counters.len) {
            _.counters[i] = 0;
            i += 1;
        }
        _.sample_size = 0;
    }

    /// Estimates frequency (`0..15`) of `key` as the minimum across all 4 hash rows.
    fn estimate(key: u64): u8 {
        const mask = _.row_width - 1;
        let min_v: u8 = TINYLFU_MAX_COUNT;
        let r: usize = 0;
        while (r < TINYLFU_ROWS) {
            const col = (hashRow(key, r) as usize) & mask;
            const v = _.readNibble(r, col);
            if (v < min_v) {
                min_v = v;
            }
            r += 1;
        }
        return min_v;
    }

    /// Increments the 4-bit saturating counters for `key`, triggering `halve()` when `sample_size >= reset_threshold`.
    /// Returns the updated frequency estimate (`0..15`).
    fn increment(*_, key: u64): u8 {
        const mask = _.row_width - 1;
        let r: usize = 0;
        while (r < TINYLFU_ROWS) {
            const col = (hashRow(key, r) as usize) & mask;
            const cur = _.readNibble(r, col);
            if (cur < TINYLFU_MAX_COUNT) {
                _.writeNibble(r, col, cur + 1);
            }
            r += 1;
        }
        _.sample_size += 1;
        if (_.sample_size >= _.reset_threshold) {
            _.halve();
        }
        return _.estimate(key);
    }

    /// W-TinyLFU admission decision: returns `true` iff `estimate(candidate) > estimate(victim)`.
    fn shouldAdmit(candidate: u64, victim: u64): bool {
        return _.estimate(candidate) > _.estimate(victim);
    }
}

/// Intrusive slot entry for `LfuCache`.
pub struct LfuSlot {
    key: u64;
    value: u64;
    freq: u64;
    last_touch: u64;
    occupied: bool;
}

/// Zero-allocation fixed-capacity LFU cache (`u64 -> u64`) with LRU tie-breaking on equal frequency.
pub struct LfuCache {
    slots: []LfuSlot;
    count: usize;
    clock: u64;

    /// Initializes an `LfuCache` backed by caller-owned `slots`. Returns `null` if `slots.len == 0`.
    pub fn init(slots: []LfuSlot): LfuCache? {
        if (slots.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < slots.len) {
            slots[i] = LfuSlot {
                key: 0,
                value: 0,
                freq: 0,
                last_touch: 0,
                occupied: false,
            };
            i += 1;
        }
        return LfuCache {
            slots: slots,
            count: 0,
            clock: 0,
        };
    }

    /// Returns number of active entries currently stored in the cache.
    fn len(): usize {
        return _.count;
    }

    /// Returns fixed maximum capacity of the cache.
    fn capacity(): usize {
        return _.slots.len;
    }

    fn findSlot(key: u64): usize? {
        let i: usize = 0;
        while (i < _.slots.len) {
            if (_.slots[i].occupied && _.slots[i].key == key) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// Reads `key` without modifying frequency or recency metadata.
    fn peek(key: u64): u64? {
        const idx = _.findSlot(key) ?? return null;
        return _.slots[idx].value;
    }

    /// Reads `key`, incrementing its frequency and updating `last_touch` on hit.
    fn get(*_, key: u64): u64? {
        const idx = _.findSlot(key) ?? return null;
        _.clock += 1;
        _.slots[idx].freq += 1;
        _.slots[idx].last_touch = _.clock;
        return _.slots[idx].value;
    }

    /// Returns current access frequency of `key`, or `null` if not present.
    fn frequencyOf(key: u64): u64? {
        const idx = _.findSlot(key) ?? return null;
        return _.slots[idx].freq;
    }

    /// Inserts or updates `(key, value)`.
    /// - Existing key: updates value, increments frequency & `last_touch` without eviction.
    /// - New key at capacity: evicts entry with minimum `freq`, breaking ties by oldest `last_touch` (LRU).
    fn put(*_, key: u64, value: u64): bool {
        if (_.slots.len == 0) {
            return false;
        }

        // Update existing key in-place
        if (idx := _.findSlot(key)) {
            _.clock += 1;
            _.slots[idx].value = value;
            _.slots[idx].freq += 1;
            _.slots[idx].last_touch = _.clock;
            return true;
        }

        // Insert into first free slot if available
        if (_.count < _.slots.len) {
            let i: usize = 0;
            while (i < _.slots.len) {
                if (!_.slots[i].occupied) {
                    _.clock += 1;
                    _.slots[i] = LfuSlot {
                        key: key,
                        value: value,
                        freq: 1,
                        last_touch: _.clock,
                        occupied: true,
                    };
                    _.count += 1;
                    return true;
                }
                i += 1;
            }
        }

        // Full capacity: select victim with lowest freq, tie-break by smallest last_touch (LRU)
        let victim: usize = 0;
        let i: usize = 1;
        while (i < _.slots.len) {
            const cur = _.slots[i];
            const best = _.slots[victim];
            if (cur.freq < best.freq || (cur.freq == best.freq && cur.last_touch < best.last_touch)) {
                victim = i;
            }
            i += 1;
        }

        _.clock += 1;
        _.slots[victim] = LfuSlot {
            key: key,
            value: value,
            freq: 1,
            last_touch: _.clock,
            occupied: true,
        };
        return true;
    }

    /// Removes `key` from the cache and frees its slot for reuse. Returns `true` if removed.
    fn remove(*_, key: u64): bool {
        const idx = _.findSlot(key) ?? return false;
        _.slots[idx] = LfuSlot {
            key: 0,
            value: 0,
            freq: 0,
            last_touch: 0,
            occupied: false,
        };
        _.count -= 1;
        return true;
    }

    /// Clears all slots and resets count and logical clock to 0.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.slots.len) {
            _.slots[i] = LfuSlot {
                key: 0,
                value: 0,
                freq: 0,
                last_touch: 0,
                occupied: false,
            };
            i += 1;
        }
        _.count = 0;
        _.clock = 0;
    }
}
