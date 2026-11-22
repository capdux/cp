// Origin: Pure-CP Stdlib (Zero-Alloc Space-Saving Heavy-Hitter Top-K Tracker & Conservative-Update Count-Min Sketch) | Created: 2026-11-22
// Pure CP zero-heap-allocation streaming frequency estimation & heavy-hitter detection engine:
//   - Metwally Space-Saving Top-K Heavy-Hitter Tracker (`HeavyHitterTracker`) with exact
//     error-bound guarantees (`est_count - error_bound <= true_freq <= est_count`)
//   - Conservative-Update Count-Min Sketch (`ConservativeCms`) over caller-owned `[]u32`
//     buffers with shard merging (`mergeFrom`) and exponential sliding-window decay (`decayHalf`)

const std = @import("std");

// ============================================================================
// Internal Deterministic Hashing & Saturating Arithmetic Helpers
// ============================================================================

const FNV1A_OFFSET_64: u64 = 0xCBF29CE484222325;
const FNV1A_PRIME_64: u64 = 0x100000001B3;
const MIX_C1: u64 = 0xBF58476D1CE4E5B9;
const MIX_C2: u64 = 0x94D049BB133111EB;
const GOLDEN_GAMMA: u64 = 0x9E3779B97F4A7C15;

fn mix64(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% MIX_C1;
    z = (z ^ (z >> 27)) *% MIX_C2;
    return z ^ (z >> 31);
}

fn hashStringWithSeed(s: string, seed: u64): u64 {
    let h: u64 = FNV1A_OFFSET_64 ^ seed;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        h = (h ^ (s[i] as u64)) *% FNV1A_PRIME_64;
    }
    return mix64(h);
}

fn saturatingAddU64(a: u64, b: u64): u64 {
    const max_u64: u64 = std.math.maxInt(u64);
    if (b > max_u64 - a) {
        return max_u64;
    }
    return a + b;
}

fn saturatingAddU32(a: u32, b: u32): u32 {
    const max_u32: u32 = std.math.maxInt(u32);
    if (b > max_u32 - a) {
        return max_u32;
    }
    return a + b;
}

fn cmsColumnForRow(h1: u64, h2: u64, row: usize, width: usize): usize {
    const row_u64: u64 = row as u64;
    const mixed: u64 = mix64(h1 ^ (row_u64 *% GOLDEN_GAMMA) ^ (h2 *% (row_u64 +% 1)));
    return (mixed % (width as u64)) as usize;
}

// ============================================================================
// 1. Space-Saving Heavy-Hitter Top-K Tracker (Metwally et al.)
// ============================================================================

/// Single tracked heavy-hitter slot in `HeavyHitterTracker`.
/// Mathematical invariant: `est_count - error_bound <= true_freq <= est_count`,
/// and `error_bound <= floor(total_weight / capacity)`.
pub struct HeavyHitterEntry {
    key_hash: u64 = 0;
    label: string = "";
    est_count: u64 = 0;
    error_bound: u64 = 0;
    occupied: bool = false;
}

/// Zero-heap-allocation Metwally Space-Saving Heavy-Hitter Top-K tracker.
/// Maintains up to `K = slots.len` monitored items in `O(K)` per observation with
/// deterministic tie-breaking and rigorous frequency error bounds.
pub struct HeavyHitterTracker {
    slots: []HeavyHitterEntry;
    used: usize = 0;
    total_weight: u64 = 0;
    evictions: u64 = 0;

    /// Initializes a zero-alloc tracker backed by caller-owned `slots_buf`.
    /// Returns `null` if `slots_buf.len == 0`.
    static fn init(slots_buf: *[]HeavyHitterEntry): HeavyHitterTracker? {
        if (slots_buf.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < slots_buf.len) : (i += 1) {
            slots_buf[i] = HeavyHitterEntry{};
        }
        return HeavyHitterTracker{
            slots: slots_buf,
            used: 0,
            total_weight: 0,
            evictions: 0,
        };
    }

    /// Observes `key` with positive `weight`.
    /// Returns `false` if `weight == 0` (no state mutation).
    fn observe(*_, key: string, weight: u64): bool {
        if (weight == 0) {
            return false;
        }
        const h: u64 = hashStringWithSeed(key, 0);
        _.total_weight = saturatingAddU64(_.total_weight, weight);

        // 1. Check if key is already monitored
        let i: usize = 0;
        while (i < _.used) : (i += 1) {
            if (_.slots[i].key_hash == h && std.mem.eql(u8, _.slots[i].label, key)) {
                _.slots[i].est_count = saturatingAddU64(_.slots[i].est_count, weight);
                return true;
            }
        }

        // 2. If capacity remains, insert into next free slot with error_bound = 0
        if (_.used < _.slots.len) {
            _.slots[_.used] = HeavyHitterEntry{
                key_hash: h,
                label: key,
                est_count: weight,
                error_bound: 0,
                occupied: true,
            };
            _.used += 1;
            return true;
        }

        // 3. Space-Saving replacement: evict slot with minimum est_count
        //    (tie-break: prefer larger error_bound, then lower slot index)
        let min_idx: usize = 0;
        let min_count: u64 = _.slots[0].est_count;
        let max_err_at_min: u64 = _.slots[0].error_bound;
        let s: usize = 1;
        while (s < _.slots.len) : (s += 1) {
            const c = _.slots[s].est_count;
            const e = _.slots[s].error_bound;
            if (c < min_count || (c == min_count && e > max_err_at_min)) {
                min_idx = s;
                min_count = c;
                max_err_at_min = e;
            }
        }

        _.slots[min_idx] = HeavyHitterEntry{
            key_hash: h,
            label: key,
            est_count: saturatingAddU64(min_count, weight),
            error_bound: min_count,
            occupied: true,
        };
        _.evictions += 1;
        return true;
    }

    /// Writes up to `min(out_entries.len, len())` tracked items sorted by
    /// `est_count` descending (tie-broken by `key_hash` ascending).
    /// Returns the number of entries written.
    fn topK(out_entries: *[]HeavyHitterEntry): usize {
        if (out_entries.len == 0 || _.used == 0) {
            return 0;
        }
        let limit: usize = out_entries.len;
        if (_.used < limit) {
            limit = _.used;
        }

        let count: usize = 0;
        let i: usize = 0;
        while (i < _.used) : (i += 1) {
            const cand = _.slots[i];
            if (count < limit) {
                let pos: usize = count;
                while (pos > 0 && (cand.est_count > out_entries[pos - 1].est_count ||
                    (cand.est_count == out_entries[pos - 1].est_count && cand.key_hash < out_entries[pos - 1].key_hash))) : (pos -= 1)
                {
                    out_entries[pos] = out_entries[pos - 1];
                }
                out_entries[pos] = cand;
                count += 1;
            } else if (cand.est_count > out_entries[limit - 1].est_count ||
                (cand.est_count == out_entries[limit - 1].est_count && cand.key_hash < out_entries[limit - 1].key_hash))
            {
                let pos: usize = limit - 1;
                while (pos > 0 && (cand.est_count > out_entries[pos - 1].est_count ||
                    (cand.est_count == out_entries[pos - 1].est_count && cand.key_hash < out_entries[pos - 1].key_hash))) : (pos -= 1)
                {
                    out_entries[pos] = out_entries[pos - 1];
                }
                out_entries[pos] = cand;
            }
        }
        return count;
    }

    /// Looks up `key` among currently tracked slots. Returns `null` if not currently tracked.
    fn lookup(key: string): HeavyHitterEntry? {
        const h: u64 = hashStringWithSeed(key, 0);
        let i: usize = 0;
        while (i < _.used) : (i += 1) {
            if (_.slots[i].key_hash == h && std.mem.eql(u8, _.slots[i].label, key)) {
                return _.slots[i];
            }
        }
        return null;
    }

    /// Returns the guaranteed lower bound (`est_count - error_bound`) for `key`, or `0` if untracked.
    fn guaranteedMinCount(key: string): u64 {
        if (entry := _.lookup(key)) {
            if (entry.est_count >= entry.error_bound) {
                return entry.est_count - entry.error_bound;
            }
        }
        return 0;
    }

    /// Returns the cumulative observed weight across all calls to `observe`.
    fn totalWeight(): u64 {
        return _.total_weight;
    }

    /// Returns the number of currently occupied slots.
    fn len(): usize {
        return _.used;
    }

    /// Returns the maximum slot capacity `K`.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Returns the total number of Space-Saving slot replacements performed.
    fn evictionCount(): u64 {
        return _.evictions;
    }

    /// Resets all slots and counters to empty state without reallocating.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            _.slots[i] = HeavyHitterEntry{};
        }
        _.used = 0;
        _.total_weight = 0;
        _.evictions = 0;
    }
}

// ============================================================================
// 2. Conservative-Update Count-Min Sketch (Estan & Varghese)
// ============================================================================

/// Dimension and hash-seed configuration for `ConservativeCms`.
pub struct CmsSketchConfig {
    depth: usize = 4;
    width: usize = 256;
    seed: u64 = 0x517CC1B727220A95;

    /// Constructs and validates a `CmsSketchConfig`.
    /// Requires `depth > 0`, `width > 0`, and `depth * width` without `usize` overflow.
    static fn init(depth: usize, width: usize, seed: u64 = 0x517CC1B727220A95): CmsSketchConfig? {
        const cfg = CmsSketchConfig{
            depth: depth,
            width: width,
            seed: seed,
        };
        if (!cfg.isValid()) {
            return null;
        }
        return cfg;
    }

    /// Validates sketch dimensions against zero and multiplication overflow.
    fn isValid(): bool {
        if (_.depth == 0 || _.width == 0) {
            return false;
        }
        if (_.depth > std.math.maxInt(usize) / _.width) {
            return false;
        }
        return true;
    }

    /// Returns the required `u32` cell count (`depth * width`), or `null` if invalid.
    fn requiredCells(): usize? {
        if (!_.isValid()) {
            return null;
        }
        return _.depth * _.width;
    }
}

/// Zero-heap-allocation Conservative-Update Count-Min Sketch over a flat `[]u32` buffer
/// of size `depth * width`. Supports conservative updates (minimizing overestimation on
/// skewed streams), distributed shard merging (`mergeFrom`), and sliding-window halving (`decayHalf`).
pub struct ConservativeCms {
    counters: []u32;
    depth: usize = 0;
    width: usize = 0;
    seed: u64 = 0;
    total_count: u64 = 0;

    /// Initializes a zero-alloc sketch over caller-owned `counters_buf`.
    /// Returns `null` if `cfg` is invalid or `counters_buf.len < cfg.depth * cfg.width`.
    static fn init(counters_buf: *[]u32, cfg: CmsSketchConfig): ConservativeCms? {
        const cells = cfg.requiredCells() ?? return null;
        if (counters_buf.len < cells) {
            return null;
        }
        let i: usize = 0;
        while (i < cells) : (i += 1) {
            counters_buf[i] = 0;
        }
        return ConservativeCms{
            counters: counters_buf[0..cells],
            depth: cfg.depth,
            width: cfg.width,
            seed: cfg.seed,
            total_count: 0,
        };
    }

    /// Adds positive `delta` to `key` using Conservative Update:
    /// computes `min_val = min_r counter[r, col_r]` and only lifts rows below `min_val + delta`.
    /// Returns `false` if `delta == 0`.
    fn add(*_, key: string, delta: u32): bool {
        if (delta == 0) {
            return false;
        }
        const h1: u64 = hashStringWithSeed(key, _.seed);
        const h2: u64 = hashStringWithSeed(key, _.seed ^ GOLDEN_GAMMA) | 1;
        const depth: usize = _.depth;
        const width: usize = _.width;

        // Pass 1: find current minimum count across all depth rows
        let min_val: u32 = std.math.maxInt(u32);
        let r: usize = 0;
        while (r < depth) : (r += 1) {
            const col: usize = cmsColumnForRow(h1, h2, r, width);
            const val: u32 = _.counters[r * width + col];
            if (val < min_val) {
                min_val = val;
            }
        }

        // Pass 2: conservatively lift only rows strictly below min_val + delta
        const target: u32 = saturatingAddU32(min_val, delta);
        let r2: usize = 0;
        while (r2 < depth) : (r2 += 1) {
            const col: usize = cmsColumnForRow(h1, h2, r2, width);
            const idx: usize = r2 * width + col;
            if (_.counters[idx] < target) {
                _.counters[idx] = target;
            }
        }

        _.total_count = saturatingAddU64(_.total_count, delta as u64);
        return true;
    }

    /// Returns the point frequency estimate `min_r counter[r, col_r]` for `key`.
    fn estimate(key: string): u32 {
        const h1: u64 = hashStringWithSeed(key, _.seed);
        const h2: u64 = hashStringWithSeed(key, _.seed ^ GOLDEN_GAMMA) | 1;

        let min_val: u32 = std.math.maxInt(u32);
        let r: usize = 0;
        while (r < _.depth) : (r += 1) {
            const col: usize = cmsColumnForRow(h1, h2, r, _.width);
            const val: u32 = _.counters[r * _.width + col];
            if (val < min_val) {
                min_val = val;
            }
        }
        return min_val;
    }

    /// Merges another compatible shard sketch into `this` via saturating element-wise addition.
    /// Returns `false` if `depth`, `width`, or `seed` differ.
    fn mergeFrom(*_, other: ConservativeCms): bool {
        if (_.depth != other.depth || _.width != other.width || _.seed != other.seed) {
            return false;
        }
        const cells: usize = _.depth * _.width;
        let i: usize = 0;
        while (i < cells) : (i += 1) {
            _.counters[i] = saturatingAddU32(_.counters[i], other.counters[i]);
        }
        _.total_count = saturatingAddU64(_.total_count, other.total_count);
        return true;
    }

    /// Applies exponential sliding-window halving (`counter >>= 1`) to all cells and `total_count`.
    fn decayHalf(*_) {
        const cells: usize = _.depth * _.width;
        let i: usize = 0;
        while (i < cells) : (i += 1) {
            _.counters[i] = _.counters[i] >> 1;
        }
        _.total_count = _.total_count >> 1;
    }

    /// Resets all counters and `total_count` to zero.
    fn reset(*_) {
        const cells: usize = _.depth * _.width;
        let i: usize = 0;
        while (i < cells) : (i += 1) {
            _.counters[i] = 0;
        }
        _.total_count = 0;
    }

    /// Returns cumulative total weight added to the sketch.
    fn totalCount(): u64 {
        return _.total_count;
    }

    /// Returns sketch row count (`depth`).
    fn rowCount(): usize {
        return _.depth;
    }

    /// Returns sketch column count (`width`).
    fn colCount(): usize {
        return _.width;
    }
}
