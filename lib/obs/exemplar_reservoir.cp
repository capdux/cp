// Origin: Pure-CP Stdlib (Zero-Alloc OpenTelemetry AlignedHistogramBucket & SimpleFixedSize Exemplar Reservoir) | Created: 2026-11-22
// Pure CP zero-heap-allocation OpenTelemetry / Prometheus Histogram Exemplar Reservoir:
//   - Custom strictly-monotonic histogram bucket boundaries with O(log B) binary-search bucket lookup
//   - Dual sampling strategies:
//     * `aligned_histogram_bucket`: per-bucket latest sampled trace exemplar (guarantees tail P99 trace retention)
//     * `simple_fixed_size`: deterministic SplitMix64 Algorithm R uniform reservoir sampling
//   - Time-window expiration (`evictOlderThan`), scrape-cycle snapshot & reset (`collectAndReset`),
//     and OpenMetrics 1.0 exemplar formatter (`formatExemplarOpenMetrics`)
//   - Strict boundary guards for non-monotonic bounds, NaN/Inf values, unsampled/zero trace IDs, and buffer limits

const std = @import("std");

/// Maximum explicit finite histogram bucket boundaries (plus 1 implicit `+Inf` bucket = 32 total).
pub const EXEMPLAR_MAX_BOUNDS: usize = 31;

/// Maximum total histogram buckets including the implicit `+Inf` overflow bucket.
pub const EXEMPLAR_MAX_BUCKETS: usize = 32;

/// Maximum inline exemplar reservoir slots (zero dynamic heap).
pub const EXEMPLAR_MAX_RESERVOIR_SLOTS: usize = 32;

/// W3C Trace Context sampled bit (`trace-flags & 0x01`).
pub const EXEMPLAR_FLAG_SAMPLED: u8 = 0x01;

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn splitMix64Next(state: *u64): u64 {
    state.* = state.* +% 0x9E3779B97F4A7C15;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn appendSlice(out: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos.* > out.len || src.len > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos.* + i] = src[i];
    }
    pos.* += src.len;
    return true;
}

fn appendHexU64(out: *[]u8, pos: *usize, v: u64): bool {
    if (pos.* > out.len || 16 > out.len - pos.*) return false;
    const hex_digits = "0123456789abcdef";
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const shift: u6 = ((15 - i) * 4) as u6;
        const nibble: usize = ((v >> shift) & 0x0F) as usize;
        out[pos.* + i] = hex_digits[nibble];
    }
    pos.* += 16;
    return true;
}

fn appendU64Dec(out: *[]u8, pos: *usize, v: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let cur: u64 = v;
    if (cur == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (cur > 0) {
            tmp[n] = (48 + (cur % 10)) as u8;
            cur /= 10;
            n += 1;
        }
    }
    if (pos.* > out.len || n > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* += n;
    return true;
}

fn appendF64Fixed6(out: *[]u8, pos: *usize, v: f64): bool {
    if (!isFiniteF64(v)) return false;
    let x: f64 = v;
    if (x < 0.0) {
        if (!appendSlice(out, pos, "-")) return false;
        x = -x;
    }
    // Scale by 1_000_000 for 6 decimal places with rounding.
    const scaled_f: f64 = x * 1000000.0 + 0.5;
    if (scaled_f > 18446744073709550000.0) return false;
    const scaled: u64 = scaled_f as u64;
    const int_part: u64 = scaled / 1000000;
    const frac_part: u64 = scaled % 1000000;

    if (!appendU64Dec(out, pos, int_part)) return false;
    if (frac_part == 0) return true;

    if (!appendSlice(out, pos, ".")) return false;
    let frac_buf: [6]u8 = [48; 6];
    let rem: u64 = frac_part;
    let idx: usize = 6;
    while (idx > 0) : (idx -= 1) {
        frac_buf[idx - 1] = (48 + (rem % 10)) as u8;
        rem /= 10;
    }
    let trim: usize = 6;
    while (trim > 1 && frac_buf[trim - 1] == 48) : (trim -= 1) {}
    return appendSlice(out, pos, frac_buf[0..trim]);
}

// ============================================================================
// 1. Public Enums, Data Models & OpenMetrics Formatter
// ============================================================================

/// Exemplar sampling strategy.
pub enum ExemplarStrategy {
    aligned_histogram_bucket,
    simple_fixed_size,
}

/// Captured OpenTelemetry / Prometheus Exemplar sample linked to a W3C Trace/Span context.
pub struct ExemplarRecord {
    value: f64 = 0.0;
    timestamp_ns: u64 = 0;
    trace_id_hi: u64 = 0;
    trace_id_lo: u64 = 0;
    span_id: u64 = 0;
    sampled_flags: u8 = 0;
    bucket_idx: u16 = 0;
    occupied: bool = false;
}

/// Per-bucket histogram statistics and exemplar occupancy snapshot.
pub struct ExemplarBucketSummary {
    upper_bound: f64 = 0.0;
    is_inf: bool = false;
    count: u64 = 0;
    sum: f64 = 0.0;
    has_exemplar: bool = false;
}

/// Aggregate operational telemetry for `AlignedHistogramExemplarReservoir`.
pub struct ExemplarReservoirStats {
    strategy: ExemplarStrategy = ExemplarStrategy.aligned_histogram_bucket,
    bucket_count: usize = 0;
    reservoir_capacity: usize = 0;
    occupied_exemplars: usize = 0;
    total_observations: u64 = 0;
    total_sum: f64 = 0.0;
    filtered_unsampled: u64 = 0;
    rejected_invalid: u64 = 0;
    evicted_expired: u64 = 0;
}

/// Formats an `ExemplarRecord` into an OpenMetrics 1.0 exemplar snippet:
/// `# {trace_id="<32-hex>",span_id="<16-hex>"} <val> <ts_sec>`
/// Returns written byte count, or `null` on unoccupied/invalid record or insufficient `out_buf`.
pub fn formatExemplarOpenMetrics(ex: ExemplarRecord, out_buf: *[]u8): usize? {
    if (!ex.occupied || !isFiniteF64(ex.value)) return null;

    let pos: usize = 0;
    if (!appendSlice(out_buf, *pos, "# {trace_id=\"")) return null;
    if (!appendHexU64(out_buf, *pos, ex.trace_id_hi)) return null;
    if (!appendHexU64(out_buf, *pos, ex.trace_id_lo)) return null;
    if (!appendSlice(out_buf, *pos, "\",span_id=\"")) return null;
    if (!appendHexU64(out_buf, *pos, ex.span_id)) return null;
    if (!appendSlice(out_buf, *pos, "\"} ")) return null;
    if (!appendF64Fixed6(out_buf, *pos, ex.value)) return null;
    if (!appendSlice(out_buf, *pos, " ")) return null;

    // Format timestamp_ns as seconds with millisecond fraction if non-zero.
    const secs: u64 = ex.timestamp_ns / 1_000_000_000;
    const ms: u64 = (ex.timestamp_ns % 1_000_000_000) / 1_000_000;
    if (!appendU64Dec(out_buf, *pos, secs)) return null;
    if (ms > 0) {
        if (!appendSlice(out_buf, *pos, ".")) return null;
        let ms_buf: [3]u8 = [
            (48 + (ms / 100)) as u8,
            (48 + ((ms / 10) % 10)) as u8,
            (48 + (ms % 10)) as u8,
        ];
        if (!appendSlice(out_buf, *pos, ms_buf[0..3])) return null;
    }
    return pos;
}

// ============================================================================
// 2. Zero-Alloc OpenTelemetry Exemplar Reservoir Engine
// ============================================================================

/// Zero-heap-allocation OpenTelemetry Histogram Exemplar Reservoir supporting both
/// `aligned_histogram_bucket` and `simple_fixed_size` (SplitMix64 Algorithm R) strategies.
pub struct AlignedHistogramExemplarReservoir {
    strategy: ExemplarStrategy = ExemplarStrategy.aligned_histogram_bucket;
    require_sampled_flag: bool = true;
    bounds: [EXEMPLAR_MAX_BOUNDS]f64 = [0.0; EXEMPLAR_MAX_BOUNDS];
    bounds_count: usize = 0;
    bucket_count: usize = 1;
    bucket_counts: [EXEMPLAR_MAX_BUCKETS]u64 = [0; EXEMPLAR_MAX_BUCKETS];
    bucket_sums: [EXEMPLAR_MAX_BUCKETS]f64 = [0.0; EXEMPLAR_MAX_BUCKETS];
    reservoir_capacity: usize = 1;
    slots: [EXEMPLAR_MAX_RESERVOIR_SLOTS]ExemplarRecord = [ExemplarRecord{}; EXEMPLAR_MAX_RESERVOIR_SLOTS];
    eligible_seen: u64 = 0;
    rng_state: u64 = 0x243F6A8885A308D3;
    total_observations: u64 = 0;
    total_sum: f64 = 0.0;
    filtered_unsampled: u64 = 0;
    rejected_invalid: u64 = 0;
    evicted_expired: u64 = 0;

    static fn validateBounds(bounds: []const f64): bool {
        if (bounds.len > EXEMPLAR_MAX_BOUNDS) return false;
        let i: usize = 0;
        while (i < bounds.len) : (i += 1) {
            if (!isFiniteF64(bounds[i])) return false;
            if (i > 0 && bounds[i] <= bounds[i - 1]) return false;
        }
        return true;
    }

    /// Initializes an `AlignedHistogramBucket` exemplar reservoir over strictly increasing `bounds`
    /// (`1..=31` finite boundaries plus 1 implicit `+Inf` bucket).
    static fn initAligned(
        bounds: []const f64,
        require_sampled_flag: bool,
    ): AlignedHistogramExemplarReservoir? {
        if (bounds.len == 0 || !AlignedHistogramExemplarReservoir.validateBounds(bounds)) {
            return null;
        }
        let res = AlignedHistogramExemplarReservoir{
            strategy: ExemplarStrategy.aligned_histogram_bucket,
            require_sampled_flag: require_sampled_flag,
            bounds_count: bounds.len,
            bucket_count: bounds.len + 1,
            reservoir_capacity: bounds.len + 1,
        };
        let i: usize = 0;
        while (i < bounds.len) : (i += 1) {
            res.bounds[i] = bounds[i];
        }
        return res;
    }

    /// Initializes a `SimpleFixedSize` uniform reservoir sampler (SplitMix64 Algorithm R) with optional histogram `bounds`.
    /// Requires `1 <= reservoir_size <= EXEMPLAR_MAX_RESERVOIR_SLOTS`.
    static fn initSimpleFixed(
        bounds: []const f64,
        reservoir_size: usize,
        rng_seed: u64,
        require_sampled_flag: bool,
    ): AlignedHistogramExemplarReservoir? {
        if (reservoir_size == 0 || reservoir_size > EXEMPLAR_MAX_RESERVOIR_SLOTS) return null;
        if (!AlignedHistogramExemplarReservoir.validateBounds(bounds)) return null;

        let res = AlignedHistogramExemplarReservoir{
            strategy: ExemplarStrategy.simple_fixed_size,
            require_sampled_flag: require_sampled_flag,
            bounds_count: bounds.len,
            bucket_count: bounds.len + 1,
            reservoir_capacity: reservoir_size,
            rng_state: if (rng_seed != 0) rng_seed else 0x243F6A8885A308D3,
        };
        let i: usize = 0;
        while (i < bounds.len) : (i += 1) {
            res.bounds[i] = bounds[i];
        }
        return res;
    }

    /// O(log B) binary search returning the smallest bucket index `i` where `value <= bounds[i]`,
    /// or `bounds_count` (`+Inf` overflow bucket) if `value` exceeds all finite bounds.
    fn findBucketIndex(value: f64): usize {
        if (_.bounds_count == 0) return 0;
        let lo: usize = 0;
        let hi: usize = _.bounds_count;
        while (lo < hi) {
            const mid: usize = lo + ((hi - lo) >> 1);
            if (value <= _.bounds[mid]) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        return lo;
    }

    /// Records a histogram observation and offers its W3C Trace/Span context to the exemplar reservoir.
    /// Rejects non-finite (`NaN`/`Inf`) values with `false`; filters out zero trace/span IDs or
    /// unsampled contexts (`sampled_flags & 0x01 == 0`) while still counting the histogram observation.
    fn observe(
        *_,
        value: f64,
        timestamp_ns: u64,
        trace_id_hi: u64,
        trace_id_lo: u64,
        span_id: u64,
        sampled_flags: u8,
    ): bool {
        if (!isFiniteF64(value)) {
            _.rejected_invalid += 1;
            return false;
        }

        const b_idx: usize = _.findBucketIndex(value);
        _.bucket_counts[b_idx] += 1;
        _.bucket_sums[b_idx] += value;
        _.total_observations += 1;
        _.total_sum += value;

        // Filter unsampled or missing trace/span contexts
        const has_trace: bool = (trace_id_hi != 0 || trace_id_lo != 0) && span_id != 0;
        const is_sampled: bool = !_.require_sampled_flag || ((sampled_flags & EXEMPLAR_FLAG_SAMPLED) != 0);
        if (!has_trace || !is_sampled) {
            _.filtered_unsampled += 1;
            return true;
        }

        const rec = ExemplarRecord{
            value: value,
            timestamp_ns: timestamp_ns,
            trace_id_hi: trace_id_hi,
            trace_id_lo: trace_id_lo,
            span_id: span_id,
            sampled_flags: sampled_flags,
            bucket_idx: b_idx as u16,
            occupied: true,
        };

        if (_.strategy == ExemplarStrategy.aligned_histogram_bucket) {
            _.slots[b_idx] = rec;
        } else {
            // Algorithm R uniform reservoir sampling with deterministic SplitMix64 PRNG
            const seen_idx: u64 = _.eligible_seen;
            _.eligible_seen += 1;
            if ((seen_idx as usize) < _.reservoir_capacity) {
                _.slots[seen_idx as usize] = rec;
            } else {
                const r: u64 = splitMix64Next(*_.rng_state) % (seen_idx + 1);
                if ((r as usize) < _.reservoir_capacity) {
                    _.slots[r as usize] = rec;
                }
            }
        }
        return true;
    }

    /// Evicts any stored exemplar whose `timestamp_ns < cutoff_ns`. Returns number of evicted exemplars.
    fn evictOlderThan(*_, cutoff_ns: u64): usize {
        let evicted: usize = 0;
        let i: usize = 0;
        while (i < _.reservoir_capacity) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].timestamp_ns < cutoff_ns) {
                _.slots[i] = ExemplarRecord{};
                evicted += 1;
            }
        }
        _.evicted_expired += evicted as u64;
        return evicted;
    }

    /// Collects all currently stored exemplars into `out_exemplars` and resets the exemplar reservoir
    /// for the next scrape interval (histogram cumulative bucket counts/sums remain intact).
    fn collectAndReset(*_, out_exemplars: *[]ExemplarRecord): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.reservoir_capacity) : (i += 1) {
            if (_.slots[i].occupied) {
                if (written < out_exemplars.len) {
                    out_exemplars[written] = _.slots[i];
                    written += 1;
                }
                _.slots[i] = ExemplarRecord{};
            }
        }
        _.eligible_seen = 0;
        return written;
    }

    /// Returns per-bucket histogram count, sum, upper bound, and exemplar occupancy for `bucket_idx`.
    fn getBucketSummary(bucket_idx: usize): ExemplarBucketSummary? {
        if (bucket_idx >= _.bucket_count) return null;
        const is_inf: bool = bucket_idx == _.bounds_count;
        const has_ex: bool = if (_.strategy == ExemplarStrategy.aligned_histogram_bucket)
            _.slots[bucket_idx].occupied
        else
            false;
        return ExemplarBucketSummary{
            upper_bound: if (is_inf) 0.0 else _.bounds[bucket_idx],
            is_inf: is_inf,
            count: _.bucket_counts[bucket_idx],
            sum: _.bucket_sums[bucket_idx],
            has_exemplar: has_ex,
        };
    }

    /// Returns aggregate reservoir statistics.
    fn stats(): ExemplarReservoirStats {
        let occ: usize = 0;
        let i: usize = 0;
        while (i < _.reservoir_capacity) : (i += 1) {
            if (_.slots[i].occupied) occ += 1;
        }
        return ExemplarReservoirStats{
            strategy: _.strategy,
            bucket_count: _.bucket_count,
            reservoir_capacity: _.reservoir_capacity,
            occupied_exemplars: occ,
            total_observations: _.total_observations,
            total_sum: _.total_sum,
            filtered_unsampled: _.filtered_unsampled,
            rejected_invalid: _.rejected_invalid,
            evicted_expired: _.evicted_expired,
        };
    }
}
