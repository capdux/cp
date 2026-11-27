// @cap/cuckoo-filter — Zero-allocation CoNEXT'14 Cuckoo Filter for CP
// Clean-room MIT implementation inspired by Fan et al. (CoNEXT'14) and Rust `cuckoofilter`.

pub const NUM_BUCKETS: usize = 256;
pub const BUCKET_SIZE: usize = 4;
pub const TOTAL_SLOTS: usize = 1024;
pub const MAX_KICKS: usize = 500;

const BUCKET_MASK: usize = 255;
const FNV64_OFFSET_BASIS: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
const FNV64_PRIME: u64 = ((0x00000100 as u64) << 32) | (0x000001b3 as u64);
const MIX_K1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
const MIX_K2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);
const DEFAULT_SEED: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);

// ---------------------------------------------------------------------------
// Internal Overflow-Safe Hashing & Fingerprint Helpers
// ---------------------------------------------------------------------------

fn mulWrap64(a: u64, b: u64) -> u64 {
    const a_lo = a & 0xFFFFFFFF;
    const a_hi = a >> 32;
    const b_lo = b & 0xFFFFFFFF;
    const b_hi = b >> 32;

    const p0 = a_lo * b_lo;
    const p1 = (a_lo * b_hi) & 0xFFFFFFFF;
    const p2 = (a_hi * b_lo) & 0xFFFFFFFF;
    const hi = ((p0 >> 32) + p1 + p2) & 0xFFFFFFFF;
    return (p0 & 0xFFFFFFFF) | (hi << 32);
}

fn avalanche64(x: u64) -> u64 {
    let z = x;
    z = mulWrap64(z ^ (z >> 30), MIX_K1);
    z = mulWrap64(z ^ (z >> 27), MIX_K2);
    return z ^ (z >> 31);
}

fn hashBytes(data: []const u8) -> u64 {
    let h = FNV64_OFFSET_BASIS;
    let i: usize = 0;
    while (i < data.len) {
        h = mulWrap64(h ^ (data[i] as u64), FNV64_PRIME);
        i += 1;
    }
    return avalanche64(h);
}

fn fingerprintFromHash(h: u64) -> u16 {
    const fp = ((h >> 48) & 0xFFFF) as u16;
    if (fp == 0) {
        return 1;
    }
    return fp;
}

/// Computes a non-zero bucket offset for `fp` so `altBucket(altBucket(i, fp), fp) == i` always holds.
fn altBucket(bucket: usize, fp: u16) -> usize {
    const fh = avalanche64(fp as u64);
    let delta = ((fh >> 32) as usize) & BUCKET_MASK;
    if (delta == 0) {
        delta = 1;
    }
    return (bucket ^ delta) & BUCKET_MASK;
}

// ---------------------------------------------------------------------------
// Public CuckooFilter Struct
// ---------------------------------------------------------------------------

pub struct CuckooFilter {
    pub buckets: [1024]u16,
    pub item_count: usize,
    pub rng_state: u64,

    /// Initializes an empty `CuckooFilter` (256 buckets x 4 slots = 1024 16-bit fingerprints).
    pub static fn init() -> CuckooFilter {
        let buckets: [1024]u16 = undefined;
        let i: usize = 0;
        while (i < TOTAL_SLOTS) {
            buckets[i] = 0;
            i += 1;
        }
        return CuckooFilter{
            buckets: buckets,
            item_count: 0,
            rng_state: DEFAULT_SEED,
        };
    }

    pub fn len() -> usize {
        return _.item_count;
    }

    pub fn capacity() -> usize {
        return TOTAL_SLOTS;
    }

    pub fn isEmpty() -> bool {
        return _.item_count == 0;
    }

    pub fn loadFactor() -> f64 {
        return (_.item_count as f64) / (TOTAL_SLOTS as f64);
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < TOTAL_SLOTS) {
            _.buckets[i] = 0;
            i += 1;
        }
        _.item_count = 0;
        _.rng_state = DEFAULT_SEED;
    }

    fn nextRand(*_) -> u64 {
        let x = _.rng_state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.rng_state = x;
        return x;
    }

    fn insertIntoBucket(*_, bucket: usize, fp: u16) -> bool {
        const base = bucket * BUCKET_SIZE;
        let s: usize = 0;
        while (s < BUCKET_SIZE) {
            if (_.buckets[base + s] == 0) {
                _.buckets[base + s] = fp;
                return true;
            }
            s += 1;
        }
        return false;
    }

    fn bucketContains(bucket: usize, fp: u16) -> bool {
        const base = bucket * BUCKET_SIZE;
        let s: usize = 0;
        while (s < BUCKET_SIZE) {
            if (_.buckets[base + s] == fp) {
                return true;
            }
            s += 1;
        }
        return false;
    }

    fn deleteFromBucket(*_, bucket: usize, fp: u16) -> bool {
        const base = bucket * BUCKET_SIZE;
        let s: usize = 0;
        while (s < BUCKET_SIZE) {
            if (_.buckets[base + s] == fp) {
                _.buckets[base + s] = 0;
                return true;
            }
            s += 1;
        }
        return false;
    }

    /// Inserts `data` into the Cuckoo filter, performing up to `MAX_KICKS` (500) relocations if needed.
    /// Returns `false` if the filter is too full to relocate.
    pub fn insert(*_, data: []const u8) -> bool {
        const h = hashBytes(data);
        const fp = fingerprintFromHash(h);
        const b1 = (h as usize) & BUCKET_MASK;
        const b2 = altBucket(b1, fp);

        if (_.insertIntoBucket(b1, fp)) {
            _.item_count += 1;
            return true;
        }
        if (_.insertIntoBucket(b2, fp)) {
            _.item_count += 1;
            return true;
        }

        // Randomly pick b1 or b2 to begin cuckoo kick-out relocation chain
        let cur_bucket = if ((_.nextRand() & 1) == 0) b1 else b2;
        let cur_fp = fp;

        let kick: usize = 0;
        while (kick < MAX_KICKS) {
            const slot = (_.nextRand() as usize) & 3;
            const idx = cur_bucket * BUCKET_SIZE + slot;
            const evicted = _.buckets[idx];
            _.buckets[idx] = cur_fp;
            cur_fp = evicted;
            cur_bucket = altBucket(cur_bucket, cur_fp);

            if (_.insertIntoBucket(cur_bucket, cur_fp)) {
                _.item_count += 1;
                return true;
            }
            kick += 1;
        }

        return false;
    }

    /// Checks whether `data` may be present in the filter (zero false negatives).
    pub fn contains(data: []const u8) -> bool {
        const h = hashBytes(data);
        const fp = fingerprintFromHash(h);
        const b1 = (h as usize) & BUCKET_MASK;
        if (_.bucketContains(b1, fp)) {
            return true;
        }
        const b2 = altBucket(b1, fp);
        return _.bucketContains(b2, fp);
    }

    /// Removes one matching fingerprint of `data` from `b1` or `b2`.
    /// Returns `true` if removed, `false` if absent.
    pub fn delete(*_, data: []const u8) -> bool {
        const h = hashBytes(data);
        const fp = fingerprintFromHash(h);
        const b1 = (h as usize) & BUCKET_MASK;
        if (_.deleteFromBucket(b1, fp)) {
            _.item_count -= 1;
            return true;
        }
        const b2 = altBucket(b1, fp);
        if (_.deleteFromBucket(b2, fp)) {
            _.item_count -= 1;
            return true;
        }
        return false;
    }
}
