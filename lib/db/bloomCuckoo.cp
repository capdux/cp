// Source: native-cp-stdlib (Bloom, Counting Bloom & Cuckoo Filters)

const std = @import("std");

pub const MAX_KICKS: usize = 500;

pub const FilterError = error {
    InvalidCapacity,
    InvalidFalsePositiveRate,
    FilterFull,
    IncompatibleFilters,
};

struct OptimalParams {
    bits: usize,
    hashes: u32,
}

fn hashBytes(data: []const u8, seed: u64): u64 {
    let h: u64 = @as(u64, 0xcbf29ce484222325) ^ seed;
    for (b of data) {
        h = h ^ @as(u64, b);
        h = h *% @as(u64, 0x100000001b3);
    }
    h = h ^ (h >> 30);
    h = h *% @as(u64, 0xbf58476d1ce4e5b9);
    h = h ^ (h >> 27);
    h = h *% @as(u64, 0x94d049bb133111eb);
    h = h ^ (h >> 31);
    return h;
}

fn computeOptimalParams(expected_items: usize, fp_rate: f64): FilterError!OptimalParams {
    if (expected_items == 0) {
        return FilterError.InvalidCapacity;
    }
    if (fp_rate <= 0.0 || fp_rate >= 1.0 || std.math.isNan(fp_rate)) {
        return FilterError.InvalidFalsePositiveRate;
    }

    const ln2: f64 = 0.6931471805599453;
    const ln2_sq: f64 = ln2 * ln2;
    const n_f: f64 = @floatFromInt(expected_items);
    const m_f: f64 = @ceil(-(n_f * @log(fp_rate)) / ln2_sq);

    let num_bits: usize = @intFromFloat(m_f);
    if (num_bits < 64) {
        num_bits = 64;
    }

    const k_f: f64 = @round((@as(f64, @floatFromInt(num_bits)) / n_f) * ln2);
    let num_hashes: u32 = @intFromFloat(k_f);
    if (num_hashes < 1) {
        num_hashes = 1;
    }

    return OptimalParams{ .bits = num_bits, .hashes = num_hashes };
}

pub struct BloomFilter {
    words: []u64,
    num_bits: usize,
    num_hashes: u32,

    pub fn initWithRate(alloc: std.mem.Allocator, expected_items: usize, fp_rate: f64): FilterError!BloomFilter {
        const params = try computeOptimalParams(expected_items, fp_rate);
        const num_words: usize = (params.bits + 63) / 64;
        const words = alloc.alloc(u64, num_words) catch return FilterError.InvalidCapacity;
        @memset(words, 0);
        return BloomFilter{
            .words = words,
            .num_bits = params.bits,
            .num_hashes = params.hashes,
        };
    }

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.words);
    }

    pub fn insert(*_, item: []const u8) {
        const h1 = hashBytes(item, 0x9e3779b97f4a7c15);
        const h2 = hashBytes(item, 0xbf58476d1ce4e5b9) | 1;
        let i: u32 = 0;
        while (i < _.num_hashes) {
            const combined: u64 = h1 +% (@as(u64, i) *% h2);
            const bit_idx: usize = @intCast(combined % @as(u64, _.num_bits));
            const word_idx: usize = bit_idx >> 6;
            const bit_off: u6 = @intCast(bit_idx & 63);
            _.words[word_idx] = _.words[word_idx] | (@as(u64, 1) << bit_off);
            i = i + 1;
        }
    }

    pub fn contains(item: []const u8): bool {
        const h1 = hashBytes(item, 0x9e3779b97f4a7c15);
        const h2 = hashBytes(item, 0xbf58476d1ce4e5b9) | 1;
        let i: u32 = 0;
        while (i < _.num_hashes) {
            const combined: u64 = h1 +% (@as(u64, i) *% h2);
            const bit_idx: usize = @intCast(combined % @as(u64, _.num_bits));
            const word_idx: usize = bit_idx >> 6;
            const bit_off: u6 = @intCast(bit_idx & 63);
            if (((_.words[word_idx] >> bit_off) & 1) == 0) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn merge(*_, other: BloomFilter): FilterError!void {
        if (_.num_bits != other.num_bits || _.num_hashes != other.num_hashes) {
            return FilterError.IncompatibleFilters;
        }
        let i: usize = 0;
        while (i < _.words.len) {
            _.words[i] = _.words[i] | other.words[i];
            i = i + 1;
        }
    }

    pub fn clear(*_) {
        @memset(_.words, 0);
    }

    pub fn bitCount(): usize {
        return _.num_bits;
    }

    pub fn hashCount(): u32 {
        return _.num_hashes;
    }
}

pub struct CountingBloomFilter {
    counters: []u8,
    num_bits: usize,
    num_hashes: u32,

    pub fn initWithRate(alloc: std.mem.Allocator, expected_items: usize, fp_rate: f64): FilterError!CountingBloomFilter {
        const params = try computeOptimalParams(expected_items, fp_rate);
        const counters = alloc.alloc(u8, params.bits) catch return FilterError.InvalidCapacity;
        @memset(counters, 0);
        return CountingBloomFilter{
            .counters = counters,
            .num_bits = params.bits,
            .num_hashes = params.hashes,
        };
    }

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.counters);
    }

    pub fn insert(*_, item: []const u8) {
        const h1 = hashBytes(item, 0x9e3779b97f4a7c15);
        const h2 = hashBytes(item, 0xbf58476d1ce4e5b9) | 1;
        let i: u32 = 0;
        while (i < _.num_hashes) {
            const combined: u64 = h1 +% (@as(u64, i) *% h2);
            const idx: usize = @intCast(combined % @as(u64, _.num_bits));
            if (_.counters[idx] < 255) {
                _.counters[idx] = _.counters[idx] + 1;
            }
            i = i + 1;
        }
    }

    pub fn remove(*_, item: []const u8): bool {
        if (!_.contains(item)) {
            return false;
        }
        const h1 = hashBytes(item, 0x9e3779b97f4a7c15);
        const h2 = hashBytes(item, 0xbf58476d1ce4e5b9) | 1;
        let i: u32 = 0;
        while (i < _.num_hashes) {
            const combined: u64 = h1 +% (@as(u64, i) *% h2);
            const idx: usize = @intCast(combined % @as(u64, _.num_bits));
            if (_.counters[idx] > 0 && _.counters[idx] < 255) {
                _.counters[idx] = _.counters[idx] - 1;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn contains(item: []const u8): bool {
        const h1 = hashBytes(item, 0x9e3779b97f4a7c15);
        const h2 = hashBytes(item, 0xbf58476d1ce4e5b9) | 1;
        let i: u32 = 0;
        while (i < _.num_hashes) {
            const combined: u64 = h1 +% (@as(u64, i) *% h2);
            const idx: usize = @intCast(combined % @as(u64, _.num_bits));
            if (_.counters[idx] == 0) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn clear(*_) {
        @memset(_.counters, 0);
    }
}

pub struct Bucket {
    slots: [4]u16,
}

fn cuckooFingerprint(item: []const u8): u16 {
    const h = hashBytes(item, 0x517cc1b727220a95);
    let fp: u16 = @intCast(h & 0xffff);
    if (fp == 0) {
        fp = 1;
    }
    return fp;
}

fn cuckooIndexHash(item: []const u8, num_buckets: usize): usize {
    const h = hashBytes(item, 0x9e3779b97f4a7c15);
    const idx: usize = @intCast(h & @as(u64, num_buckets - 1));
    return idx;
}

fn cuckooAltIndex(idx: usize, fp: u16, num_buckets: usize): usize {
    let fh: u64 = @as(u64, fp) *% @as(u64, 0xbf58476d1ce4e5b9);
    fh = fh ^ (fh >> 27);
    fh = fh *% @as(u64, 0x94d049bb133111eb);
    fh = fh ^ (fh >> 31);
    const offset: usize = @intCast(fh & @as(u64, num_buckets - 1));
    return idx ^ offset;
}

pub struct CuckooFilter {
    buckets: []Bucket,
    num_buckets: usize,
    item_count: usize,
    rng_state: u64,

    pub static fn init(alloc: std.mem.Allocator, capacity: usize): FilterError!CuckooFilter {
        if (capacity == 0) {
            return FilterError.InvalidCapacity;
        }
        const min_buckets: usize = (capacity + 3) / 4;
        let num_buckets: usize = 2;
        while (num_buckets < min_buckets) {
            num_buckets = num_buckets * 2;
        }
        let buckets = alloc.alloc(Bucket, num_buckets) catch return FilterError.InvalidCapacity;
        let i: usize = 0;
        while (i < num_buckets) {
            buckets[i] = Bucket{ .slots = .{ 0, 0, 0, 0 } };
            i = i + 1;
        }
        return CuckooFilter{
            .buckets = buckets,
            .num_buckets = num_buckets,
            .item_count = 0,
            .rng_state = 0x9e3779b97f4a7c15,
        };
    }

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.buckets);
    }

    fn nextRand(*_): u64 {
        let x: u64 = _.rng_state;
        x = x ^ (x << 13);
        x = x ^ (x >> 7);
        x = x ^ (x << 17);
        _.rng_state = x;
        return x;
    }

    fn insertIntoBucket(*_, bucket_idx: usize, fp: u16): bool {
        let s: usize = 0;
        while (s < 4) {
            if (_.buckets[bucket_idx].slots[s] == 0) {
                _.buckets[bucket_idx].slots[s] = fp;
                return true;
            }
            s = s + 1;
        }
        return false;
    }

    pub fn insert(*_, item: []const u8): FilterError!void {
        const fp = cuckooFingerprint(item);
        const i1 = cuckooIndexHash(item, _.num_buckets);
        const i2 = cuckooAltIndex(i1, fp, _.num_buckets);

        if (_.insertIntoBucket(i1, fp)) {
            _.item_count = _.item_count + 1;
            return;
        }
        if (_.insertIntoBucket(i2, fp)) {
            _.item_count = _.item_count + 1;
            return;
        }

        let cur_idx: usize = i1;
        if ((_.nextRand() & 1) == 1) {
            cur_idx = i2;
        }
        let cur_fp: u16 = fp;

        let kick: usize = 0;
        while (kick < MAX_KICKS) {
            const slot_idx: usize = @intCast(_.nextRand() & 3);
            const evicted: u16 = _.buckets[cur_idx].slots[slot_idx];
            _.buckets[cur_idx].slots[slot_idx] = cur_fp;
            cur_fp = evicted;
            cur_idx = cuckooAltIndex(cur_idx, cur_fp, _.num_buckets);

            if (_.insertIntoBucket(cur_idx, cur_fp)) {
                _.item_count = _.item_count + 1;
                return;
            }
            kick = kick + 1;
        }

        return FilterError.FilterFull;
    }

    pub fn contains(item: []const u8): bool {
        const fp = cuckooFingerprint(item);
        const i1 = cuckooIndexHash(item, _.num_buckets);
        const i2 = cuckooAltIndex(i1, fp, _.num_buckets);

        let s: usize = 0;
        while (s < 4) {
            if (_.buckets[i1].slots[s] == fp || _.buckets[i2].slots[s] == fp) {
                return true;
            }
            s = s + 1;
        }
        return false;
    }

    pub fn remove(*_, item: []const u8): bool {
        const fp = cuckooFingerprint(item);
        const i1 = cuckooIndexHash(item, _.num_buckets);
        const i2 = cuckooAltIndex(i1, fp, _.num_buckets);

        let s: usize = 0;
        while (s < 4) {
            if (_.buckets[i1].slots[s] == fp) {
                _.buckets[i1].slots[s] = 0;
                _.item_count = _.item_count - 1;
                return true;
            }
            if (_.buckets[i2].slots[s] == fp) {
                _.buckets[i2].slots[s] = 0;
                _.item_count = _.item_count - 1;
                return true;
            }
            s = s + 1;
        }
        return false;
    }

    pub fn count(): usize {
        return _.item_count;
    }
}
