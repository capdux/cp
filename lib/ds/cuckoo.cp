// Source: cp/lib/cuckoo/cuckoo.cp (Pure CP 4-Slot 16-bit Fingerprint Cuckoo Filter)
// Zero-C-dependency probabilistic membership filter supporting O(1) lookup,
// partial-key XOR relocation (altIndex = i ^ hash(fp)), rollback-safe eviction,
// and native O(1) safe element deletion.

const std = @import("std");

const BUCKET_SIZE: usize = 4;
const MAX_KICKS: usize = 500;

/// Error set for CuckooFilter operations.
pub err CuckooError {
    FilterFull,
    InvalidCapacity,
    OutOfMemory,
}

/// Runtime statistics snapshot for CuckooFilter.
pub struct CuckooStats {
    bucket_count: usize,
    slot_count: usize,
    item_count: usize,
    load_factor: f64,
    total_kicks: u64,
}

struct KickRecord {
    bucket: usize,
    slot: usize,
    old_fp: u16,
}

fn nextPowerOfTwo(n: usize): usize {
    if (n <= 1) return 1;
    let p: usize = 1;
    while (p < n) {
        if (p > (std.math.maxInt(usize) >> 1)) {
            return 0;
        }
        p <<= 1;
    }
    return p;
}

fn hashBytes(data: []const u8): u64 {
    // 64-bit Wyhash for fast, uniform avalanche over arbitrary byte slices.
    return std.hash.Wyhash.hash(0, data);
}

fn hashFingerprint(fp: u16): usize {
    // SplitMix64/Murmur3-style mixer so altIndex = (i ^ hash(fp)) & mask is well-distributed and strictly involutive.
    let x: u64 = (fp as u64) *% 0x9e3779b97f4a7c15;
    x = (x ^ (x >> 30)) *% 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) *% 0x94d049bb133111eb;
    x ^= x >> 31;
    return x as usize;
}

fn extractFingerprint(h: u64): u16 {
    // 0 is reserved as the empty-slot sentinel; map 0 to 1.
    const raw = ((h >> 32) & 0xFFFF) as u16;
    if (raw == 0) return 1;
    return raw;
}

/// 4-slot-per-bucket, 16-bit non-zero fingerprint Cuckoo Filter.
pub struct CuckooFilter {
    alloc: std.mem.Allocator,
    slots: []u16,
    bucket_count: usize,
    item_count: usize,
    total_kicks: u64,
    prng_state: u64,

    /// Initialize a CuckooFilter with at least `min_capacity` total slots (rounded up to power-of-two buckets).
    pub static fn init(alloc: std.mem.Allocator, min_capacity: usize): CuckooFilter ! CuckooError {
        if (min_capacity == 0) {
            return CuckooError.InvalidCapacity;
        }
        const min_buckets = (min_capacity + BUCKET_SIZE - 1) / BUCKET_SIZE;
        const bucket_count = nextPowerOfTwo(min_buckets);
        if (bucket_count == 0 || bucket_count > (std.math.maxInt(usize) / BUCKET_SIZE)) {
            return CuckooError.InvalidCapacity;
        }
        const slot_count = bucket_count * BUCKET_SIZE;
        const slots = alloc.alloc(u16, slot_count) catch return CuckooError.OutOfMemory;
        @memset(slots, 0);
        return CuckooFilter{
            alloc: alloc,
            slots: slots,
            bucket_count: bucket_count,
            item_count: 0,
            total_kicks: 0,
            prng_state: 0x243f6a8885a308d3,
        };
    }

    /// Release backing slot storage.
    pub fn deinit(*_) {
        if (_.slots.len > 0) {
            _.alloc.free(_.slots);
            _.slots = [];
        }
        _.bucket_count = 0;
        _.item_count = 0;
    }

    fn nextRand(*_): u64 {
        // Xorshift64* PRNG for random victim slot selection during cuckoo eviction.
        let x = _.prng_state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.prng_state = x;
        return x *% 0x2545f4914f6cdd1d;
    }

    fn altIndex(bucket_idx: usize, fp: u16): usize {
        const mask = _.bucket_count - 1;
        return (bucket_idx ^ hashFingerprint(fp)) & mask;
    }

    fn bucketHasFp(bucket_idx: usize, fp: u16): bool {
        const base = bucket_idx * BUCKET_SIZE;
        let s: usize = 0;
        while (s < BUCKET_SIZE) : (s += 1) {
            if (_.slots[base + s] == fp) {
                return true;
            }
        }
        return false;
    }

    fn tryInsertIntoBucket(*_, bucket_idx: usize, fp: u16): bool {
        const base = bucket_idx * BUCKET_SIZE;
        let s: usize = 0;
        while (s < BUCKET_SIZE) : (s += 1) {
            if (_.slots[base + s] == 0) {
                _.slots[base + s] = fp;
                return true;
            }
        }
        return false;
    }

    fn tryDeleteFromBucket(*_, bucket_idx: usize, fp: u16): bool {
        const base = bucket_idx * BUCKET_SIZE;
        let s: usize = 0;
        while (s < BUCKET_SIZE) : (s += 1) {
            if (_.slots[base + s] == fp) {
                _.slots[base + s] = 0;
                return true;
            }
        }
        return false;
    }

    /// Insert `item` into the filter. Performs up to MAX_KICKS (500) relocations;
    /// rolls back cleanly and returns `CuckooError.FilterFull` if no empty slot is found.
    pub fn insert(*_, item: []const u8): void ! CuckooError {
        if (_.bucket_count == 0) {
            return CuckooError.InvalidCapacity;
        }
        const h = hashBytes(item);
        const fp = extractFingerprint(h);
        const mask = _.bucket_count - 1;
        const b1 = (h as usize) & mask;
        const b2 = _.altIndex(b1, fp);

        if (_.tryInsertIntoBucket(b1, fp) || _.tryInsertIntoBucket(b2, fp)) {
            _.item_count += 1;
            return;
        }

        // Both candidate buckets are full; start bounded random-walk eviction with rollback log.
        let kick_log: [MAX_KICKS]KickRecord = undefined;
        let cur_bucket = if ((_.nextRand() & 1) == 0) b1 else b2;
        let cur_fp = fp;

        let k: usize = 0;
        while (k < MAX_KICKS) : (k += 1) {
            const victim_slot = (_.nextRand() & 3) as usize;
            const slot_offset = cur_bucket * BUCKET_SIZE + victim_slot;
            const evicted_fp = _.slots[slot_offset];

            kick_log[k] = KickRecord{
                bucket: cur_bucket,
                slot: victim_slot,
                old_fp: evicted_fp,
            };
            _.slots[slot_offset] = cur_fp;

            const next_bucket = _.altIndex(cur_bucket, evicted_fp);
            if (_.tryInsertIntoBucket(next_bucket, evicted_fp)) {
                _.item_count += 1;
                _.total_kicks += (k + 1) as u64;
                return;
            }

            cur_bucket = next_bucket;
            cur_fp = evicted_fp;
        }

        // Rollback in reverse order so zero existing items are ever lost on FilterFull.
        let r: usize = MAX_KICKS;
        while (r > 0) {
            r -= 1;
            const rec = kick_log[r];
            _.slots[rec.bucket * BUCKET_SIZE + rec.slot] = rec.old_fp;
        }
        return CuckooError.FilterFull;
    }

    /// Insert `item` only if `contains(item)` is currently false.
    /// Returns `true` if inserted, `false` if already present.
    pub fn insertUnique(*_, item: []const u8): bool ! CuckooError {
        if (_.contains(item)) {
            return false;
        }
        _.insert(item)?;
        return true;
    }

    /// Check whether `item` may be present in the filter (zero false negatives).
    pub fn contains(item: []const u8): bool {
        if (_.bucket_count == 0) return false;
        const h = hashBytes(item);
        const fp = extractFingerprint(h);
        const mask = _.bucket_count - 1;
        const b1 = (h as usize) & mask;
        if (_.bucketHasFp(b1, fp)) return true;
        const b2 = _.altIndex(b1, fp);
        return _.bucketHasFp(b2, fp);
    }

    /// Remove one matching fingerprint of `item` from either candidate bucket.
    /// Returns `true` if a matching fingerprint was removed, `false` otherwise.
    pub fn delete(*_, item: []const u8): bool {
        if (_.bucket_count == 0) return false;
        const h = hashBytes(item);
        const fp = extractFingerprint(h);
        const mask = _.bucket_count - 1;
        const b1 = (h as usize) & mask;
        if (_.tryDeleteFromBucket(b1, fp)) {
            _.item_count -= 1;
            return true;
        }
        const b2 = _.altIndex(b1, fp);
        if (_.tryDeleteFromBucket(b2, fp)) {
            _.item_count -= 1;
            return true;
        }
        return false;
    }

    /// Clear all slots and reset counters while retaining allocated capacity.
    pub fn clear(*_) {
        if (_.slots.len > 0) {
            @memset(_.slots, 0);
        }
        _.item_count = 0;
        _.total_kicks = 0;
    }

    /// Return current number of inserted items.
    pub fn len(): usize {
        return _.item_count;
    }

    /// Return current slot occupancy ratio in [0.0, 1.0].
    pub fn loadFactor(): f64 {
        if (_.slots.len == 0) return 0.0;
        return (_.item_count as f64) / (_.slots.len as f64);
    }

    /// Return a snapshot of filter capacity, occupancy, and relocation statistics.
    pub fn stats(): CuckooStats {
        return CuckooStats{
            bucket_count: _.bucket_count,
            slot_count: _.slots.len,
            item_count: _.item_count,
            load_factor: _.loadFactor(),
            total_kicks: _.total_kicks,
        };
    }
}
