// Reference-hardened against HdrHistogram/HdrHistogram_rust (MIT OR Apache-2.0) & OpenMetrics 1.0
// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc HDR Log-Linear High-Dynamic-Range Latency Histogram) | Created: 2026-11-22
// Pure CP zero-heap-allocation HdrHistogram-style log-linear latency histogram with constant memory,
// O(1) bit-level bucket mapping (~1.56% max relative error across microsecond-to-hour dynamic range),
// Coordinated Omission correction, saturating u64 overflow protection, and 4-lane SIMD bucket aggregation:
//   - 4-lane @Vector(4, u64) SIMD (`HdrVec4u64`) saturating merge (`simdHdrMergeBuckets`) and
//     saturating total sample reduction (`simdHdrSumBuckets`) with 0-diff scalar references
//   - Exact 1:1 linear base band for [0, 127], followed by 28 binary log-linear scales (64 sub-buckets/scale)
//   - Covers [0, 34_359_738_367] (~9.5 hours in microseconds) in fixed 1920 u64 inline buckets
//   - `record_with_expected_interval` backfills hidden stalled samples (Coordinated Omission correction)
//   - `merge_from` / `merge` supports saturating cross-worker/cross-thread aggregation

const std = @import("std");

/// Module-local 4-lane u64 vector alias for HDR histogram bucket hot paths.
type HdrVec4u64 = @Vector(4, u64);

// ============================================================================
// 1. Histogram Geometry Constants
// ============================================================================

/// Sub-buckets per binary magnitude scale (64 -> <= 1/64 = 1.5625% relative precision).
pub const HDR_SUB_BUCKETS: usize = 64;
/// Number of logarithmic doubling scales above the exact [0, 127] linear base band.
pub const HDR_NUM_SCALES: usize = 28;
/// Total fixed inline bucket count: 2 * 64 linear exact buckets + 28 * 64 log-linear buckets = 1920.
pub const HDR_TOTAL_BUCKETS: usize = 1920;
/// Highest value representable without clamping: (128 << 28) - 1 = 34,359,738,367.
pub const HDR_MAX_SUPPORTED_VALUE: u64 = ((128 as u64) << 28) - 1;

const HDR_LINEAR_LIMIT: u64 = 128;
const HDR_MAX_CO_BACKFILL_STEPS: usize = 100_000;
const HDR_U64_MAX: u64 = std.math.maxInt(u64);

// ============================================================================
// 2. 4-Lane SIMD Saturating Bucket Aggregation Primitives (0-Diff with Scalar)
// ============================================================================

fn hdrSatAddU64(a: u64, b: u64): u64 {
    return if (a > HDR_U64_MAX - b) HDR_U64_MAX else (a + b);
}

/// Scalar reference: saturating element-wise bucket addition (`dst[i] = sat_add(dst[i], src[i])`).
pub fn scalarHdrMergeBuckets(dst: *[]u64, src: []const u64) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = hdrSatAddU64(dst[i], src[i]);
    }
}

/// 4-lane SIMD accelerated saturating bucket merge (`dst[i] = sat_add(dst[i], src[i])`).
pub fn simdHdrMergeBuckets(dst: *[]u64, src: []const u64) {
    const n = if (dst.len < src.len) dst.len else src.len;
    const max_v: HdrVec4u64 = @splat(HDR_U64_MAX);
    let i: usize = 0;
    while (i + 4 <= n) {
        const vd: HdrVec4u64 = dst[i .. i + 4][0..4].*;
        const vs: HdrVec4u64 = src[i .. i + 4][0..4].*;
        const sum = vd +% vs;
        const overflow = sum < vd;
        dst[i .. i + 4][0..4].* = @select(u64, overflow, max_v, sum);
        i += 4;
    }
    while (i < n) : (i += 1) {
        dst[i] = hdrSatAddU64(dst[i], src[i]);
    }
}

/// Scalar reference: saturating total sample count across all histogram buckets.
pub fn scalarHdrSumBuckets(buckets: []const u64): u64 {
    let total: u64 = 0;
    let i: usize = 0;
    while (i < buckets.len) : (i += 1) {
        total = hdrSatAddU64(total, buckets[i]);
    }
    return total;
}

/// 4-lane SIMD accelerated saturating total sample count across all histogram buckets.
pub fn simdHdrSumBuckets(buckets: []const u64): u64 {
    const max_v: HdrVec4u64 = @splat(HDR_U64_MAX);
    let acc_v: HdrVec4u64 = @splat(0);
    let i: usize = 0;
    while (i + 4 <= buckets.len) {
        const v: HdrVec4u64 = buckets[i .. i + 4][0..4].*;
        const sum = acc_v +% v;
        const overflow = sum < acc_v;
        acc_v = @select(u64, overflow, max_v, sum);
        i += 4;
    }
    const lanes: [4]u64 = acc_v;
    let total: u64 = hdrSatAddU64(hdrSatAddU64(lanes[0], lanes[1]), hdrSatAddU64(lanes[2], lanes[3]));
    while (i < buckets.len) : (i += 1) {
        total = hdrSatAddU64(total, buckets[i]);
    }
    return total;
}

// ============================================================================
// 3. Quantile Summary Model
// ============================================================================

/// Comprehensive latency distribution summary produced by `HdrHistogram.summarize`.
pub struct HdrQuantileSummary {
    total_count: u64 = 0;
    min_val: u64 = 0;
    max_val: u64 = 0;
    mean_f64: f64 = 0.0;
    p50: u64 = 0;
    p90: u64 = 0;
    p95: u64 = 0;
    p99: u64 = 0;
    p999: u64 = 0;
}

// ============================================================================
// 4. Zero-Alloc HdrHistogram Engine
// ============================================================================

/// Zero-heap-allocation log-linear high-dynamic-range histogram.
pub struct HdrHistogram {
    buckets: [HDR_TOTAL_BUCKETS]u64 = [0; HDR_TOTAL_BUCKETS];
    max_trackable_val: u64 = HDR_MAX_SUPPORTED_VALUE;
    total_count: u64 = 0;
    clamped_count: u64 = 0;
    min_val: u64 = 0;
    max_val: u64 = 0;
    sum_f64: f64 = 0.0;

    /// Initializes an empty `HdrHistogram` configured with `max_trackable_val`.
    /// Values `0` or exceeding `HDR_MAX_SUPPORTED_VALUE` are clamped to `HDR_MAX_SUPPORTED_VALUE`.
    static fn init(max_trackable_val: u64): HdrHistogram {
        const effective_max = if (max_trackable_val == 0 || max_trackable_val > HDR_MAX_SUPPORTED_VALUE)
            HDR_MAX_SUPPORTED_VALUE
        else
            max_trackable_val;
        return HdrHistogram{
            buckets: [0; HDR_TOTAL_BUCKETS],
            max_trackable_val: effective_max,
            total_count: 0,
            clamped_count: 0,
            min_val: 0,
            max_val: 0,
            sum_f64: 0.0,
        };
    }

    /// Maps any `u64` value to its O(1) log-linear bucket index using pure integer bit operations.
    /// - `[0, 127]` map 1:1 to buckets `[0, 127]` (zero quantization error).
    /// - `[128, HDR_MAX_SUPPORTED_VALUE]` map across 28 binary scales with 64 sub-buckets each.
    /// - Values above `HDR_MAX_SUPPORTED_VALUE` clamp to `HDR_TOTAL_BUCKETS - 1`.
    static fn value_to_bucket(val: u64): usize {
        if (val < HDR_LINEAR_LIMIT) {
            return val as usize;
        }
        const lz: u64 = @clz(val) as u64;
        const msb: u64 = (63 as u64) - lz; // >= 7 since val >= 128
        const shift: u64 = msb - (6 as u64); // >= 1
        if (shift > (HDR_NUM_SCALES as u64)) {
            return HDR_TOTAL_BUCKETS - 1;
        }
        const shifted: u64 = val >> (shift as u6);
        const sub: usize = (shifted - (HDR_SUB_BUCKETS as u64)) as usize;
        return 128 + ((shift as usize) - 1) * HDR_SUB_BUCKETS + sub;
    }

    /// Returns the conservative inclusive upper bound of `bucket_idx` using pure integer bit operations.
    /// Guaranteed invariant: `val <= bucket_upper_bound(value_to_bucket(val))` for all representable `val`.
    static fn bucket_upper_bound(bucket_idx: usize): u64 {
        const clamped_idx = if (bucket_idx >= HDR_TOTAL_BUCKETS) HDR_TOTAL_BUCKETS - 1 else bucket_idx;
        if (clamped_idx < 128) {
            return clamped_idx as u64;
        }
        const rel = clamped_idx - 128;
        const shift: u6 = ((rel / HDR_SUB_BUCKETS) + 1) as u6;
        const sub: u64 = (rel % HDR_SUB_BUCKETS) as u64;
        const low: u64 = ((HDR_SUB_BUCKETS as u64) + sub) << shift;
        const width: u64 = (1 as u64) << shift;
        return low + width - 1;
    }

    /// Records a single observation `val`.
    /// If `val > max_trackable_val`, records at `max_trackable_val`, increments `clamped_count`, and returns `false`.
    fn record(*_, val: u64): bool {
        return _.record_n(val, 1);
    }

    /// Alias for `record` matching `HdrHistogram_rust` naming (`recordValue`).
    fn recordValue(*_, val: u64): bool {
        return _.record_n(val, 1);
    }

    /// Records `count` identical observations of `val` in O(1) with saturating u64 overflow protection.
    /// Returns `false` if `val` exceeded `max_trackable_val` (clamped and counted in `clamped_count`).
    fn record_n(*_, val: u64, count: u64): bool {
        if (count == 0) {
            return true;
        }
        const was_clamped = val > _.max_trackable_val;
        const eff_val = if (was_clamped) _.max_trackable_val else val;
        const bucket = HdrHistogram.value_to_bucket(eff_val);

        _.buckets[bucket] = hdrSatAddU64(_.buckets[bucket], count);
        if (_.total_count == 0) {
            _.min_val = eff_val;
            _.max_val = eff_val;
        } else {
            if (eff_val < _.min_val) {
                _.min_val = eff_val;
            }
            if (eff_val > _.max_val) {
                _.max_val = eff_val;
            }
        }
        _.total_count = hdrSatAddU64(_.total_count, count);
        if (was_clamped) {
            _.clamped_count = hdrSatAddU64(_.clamped_count, count);
        }
        _.sum_f64 += (eff_val as f64) * (count as f64);
        return !was_clamped;
    }

    /// Records `val` with Coordinated Omission correction: if `val > expected_interval` (and
    /// `expected_interval > 0`), backfills synthetic delayed samples at `val - k * expected_interval`
    /// down to `expected_interval` so stalled queues are faithfully reflected in tail percentiles.
    fn record_with_expected_interval(*_, val: u64, expected_interval: u64): bool {
        const ok = _.record(val);
        if (expected_interval == 0 || val <= expected_interval) {
            return ok;
        }
        let missing: u64 = val - expected_interval;
        let steps: usize = 0;
        while (missing >= expected_interval && steps < HDR_MAX_CO_BACKFILL_STEPS) : (steps += 1) {
            * = _.record(missing);
            missing -= expected_interval;
        }
        return ok;
    }

    /// Returns the latency value at quantile `q in [0.0, 1.0]` (e.g., `0.50`, `0.99`, `0.999`).
    /// Returns `0` for empty histograms or invalid (`NaN`, `Inf`, `< 0.0`, `> 1.0`) quantiles.
    /// Guarantees monotonicity: `min_val <= value_at_quantile(q1) <= value_at_quantile(q2) <= max_val` for `q1 <= q2`.
    fn value_at_quantile(q: f64): u64 {
        if (_.total_count == 0 || std.math.isNan(q) || std.math.isInf(q) || q < 0.0 || q > 1.0) {
            return 0;
        }
        if (q == 0.0) {
            return _.min_val;
        }
        if (q == 1.0) {
            return _.max_val;
        }

        const raw_rank: f64 = std.math.ceil(q * (_.total_count as f64));
        let target_rank: u64 = if (raw_rank >= (_.total_count as f64))
            _.total_count
        else
            (raw_rank as u64);
        if (target_rank == 0) {
            target_rank = 1;
        }

        let cumulative: u64 = 0;
        let b: usize = 0;
        while (b < HDR_TOTAL_BUCKETS) : (b += 1) {
            const c = _.buckets[b];
            if (c == 0) {
                continue;
            }
            cumulative = hdrSatAddU64(cumulative, c);
            if (cumulative >= target_rank) {
                let upper = HdrHistogram.bucket_upper_bound(b);
                if (upper < _.min_val) upper = _.min_val;
                if (upper > _.max_val) upper = _.max_val;
                return upper;
            }
        }
        return _.max_val;
    }

    /// Produces a full summary containing count, min, max, mean, and P50/P90/P95/P99/P99.9.
    fn summarize(): HdrQuantileSummary {
        if (_.total_count == 0) {
            return HdrQuantileSummary{};
        }
        return HdrQuantileSummary{
            total_count: _.total_count,
            min_val: _.min_val,
            max_val: _.max_val,
            mean_f64: _.sum_f64 / (_.total_count as f64),
            p50: _.value_at_quantile(0.50),
            p90: _.value_at_quantile(0.90),
            p95: _.value_at_quantile(0.95),
            p99: _.value_at_quantile(0.99),
            p999: _.value_at_quantile(0.999),
        };
    }

    /// Merges all bucket counts and summary aggregates from `other` into this histogram using
    /// 4-lane SIMD saturating addition (`simdHdrMergeBuckets`) and `simdHdrSumBuckets`.
    /// Returns `false` if `max_trackable_val` configurations differ.
    fn merge_from(*_, other: HdrHistogram): bool {
        if (_.max_trackable_val != other.max_trackable_val) {
            return false;
        }
        if (other.total_count == 0) {
            return true;
        }
        simdHdrMergeBuckets(_.buckets[0..], other.buckets[0..]);
        if (_.total_count == 0) {
            _.min_val = other.min_val;
            _.max_val = other.max_val;
        } else {
            if (other.min_val < _.min_val) {
                _.min_val = other.min_val;
            }
            if (other.max_val > _.max_val) {
                _.max_val = other.max_val;
            }
        }
        _.total_count = simdHdrSumBuckets(_.buckets[0..]);
        _.clamped_count = hdrSatAddU64(_.clamped_count, other.clamped_count);
        _.sum_f64 += other.sum_f64;
        return true;
    }

    /// Alias for `merge_from` matching `HdrHistogram_rust` naming (`merge`).
    fn merge(*_, other: HdrHistogram): bool {
        return _.merge_from(other);
    }

    /// Clears all recorded samples while preserving `max_trackable_val`.
    fn reset(*_) {
        let i: usize = 0;
        while (i < HDR_TOTAL_BUCKETS) : (i += 1) {
            _.buckets[i] = 0;
        }
        _.total_count = 0;
        _.clamped_count = 0;
        _.min_val = 0;
        _.max_val = 0;
        _.sum_f64 = 0.0;
    }
}
