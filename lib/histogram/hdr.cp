// Source: cp/lib/histogram/hdr.cp (HDR Histogram High-Dynamic-Range Latency Distribution)
// Pure CP zero-C-dependency Gil Tene style High Dynamic Range (HDR) Histogram
// with logarithmic-linear sub-bucketing (1..5 significant digits), Coordinated Omission
// correction, lossless distributed merge, exact percentile/stddev queries, and iterator.

const std = @import("std");

/// Error set for HDR histogram, EWMA, and sliding window operations.
pub err HistogramError {
    InvalidRange,
    InvalidPrecision,
    ValueOutOfRange,
    IncompatibleHistogram,
    InvalidWindow,
    OutOfMemory,
}

/// Single non-empty bucket yielded by `HdrHistogram.iterRecorded()`.
pub struct RecordedBucket {
    value_from: u64 = 0;
    value_to: u64 = 0;
    median_value: u64 = 0;
    count: u64 = 0;
    cumulative_count: u64 = 0;
    percentile: f64 = 0.0;
}

/// Zero-allocation iterator over recorded non-zero buckets in ascending value order.
pub struct RecordedIter {
    counts: []const u64,
    sub_bucket_count: usize,
    sub_bucket_half_count: usize,
    sub_bucket_half_count_magnitude: u6,
    unit_magnitude: u6,
    total_count: u64,
    pos: usize = 0,
    cumulative_count: u64 = 0,

    /// Advance to the next non-zero bucket, or return `null` when exhausted.
    pub fn next(*_): RecordedBucket? {
        while (_.pos < _.counts.len) {
            const idx = _.pos;
            _.pos += 1;
            const c = _.counts[idx];
            if (c == 0) {
                continue;
            }
            _.cumulative_count += c;

            let bucket_idx: usize = 0;
            let sub_idx: usize = idx;
            if (idx >= _.sub_bucket_count) {
                bucket_idx = (idx >> _.sub_bucket_half_count_magnitude) - 1;
                const mask = _.sub_bucket_half_count - 1;
                sub_idx = (idx & mask) + _.sub_bucket_half_count;
            }

            const shift = @as(u6, @intCast(bucket_idx)) + _.unit_magnitude;
            const unit_size: u64 = @as(u64, 1) << shift;
            const value_from: u64 = (@as(u64, sub_idx)) << shift;
            const value_to: u64 = value_from + unit_size - 1;
            const median_value: u64 = value_from + (unit_size >> 1);
            let pct: f64 = 0.0;
            if (_.total_count > 0) {
                pct = ((_.cumulative_count as f64) * 100.0) / (_.total_count as f64);
            }
            return RecordedBucket{
                value_from: value_from,
                value_to: value_to,
                median_value: median_value,
                count: c,
                cumulative_count: _.cumulative_count,
                percentile: pct,
            };
        }
        return null;
    }
}

/// High Dynamic Range (HDR) Histogram supporting 1..5 significant digits of precision
/// across arbitrary microsecond-to-hour dynamic ranges with bounded memory footprint.
pub struct HdrHistogram {
    alloc: std.mem.Allocator,
    lowest_discernible_value: u64,
    highest_trackable_value: u64,
    significant_digits: u8,
    unit_magnitude: u6,
    sub_bucket_count_magnitude: u6,
    sub_bucket_half_count_magnitude: u6,
    sub_bucket_count: usize,
    sub_bucket_half_count: usize,
    sub_bucket_mask: u64,
    leading_zero_count_base: u7,
    bucket_count: usize,
    counts: []u64,
    total_count: u64,
    min_value: u64,
    max_value: u64,

    /// Initialize an HDR Histogram covering `[lowest_discernible_value, highest_trackable_value]`
    /// with `significant_digits` in `1..5`.
    pub static fn init(
        alloc: std.mem.Allocator,
        lowest_discernible_value: u64,
        highest_trackable_value: u64,
        significant_digits: u8,
    ): HdrHistogram ! HistogramError {
        if (significant_digits < 1 || significant_digits > 5) {
            return HistogramError.InvalidPrecision;
        }
        if (lowest_discernible_value < 1 || highest_trackable_value < lowest_discernible_value * 2) {
            return HistogramError.InvalidRange;
        }

        // unit_magnitude = floor(log2(lowest_discernible_value))
        const unit_mag_u7: u7 = @as(u7, 63) - @as(u7, @clz(lowest_discernible_value));
        if (unit_mag_u7 >= 64) {
            return HistogramError.InvalidRange;
        }
        const unit_magnitude: u6 = @as(u6, @intCast(unit_mag_u7));

        // largest_value_with_single_unit_resolution = 2 * 10^significant_digits
        let largest_single_unit_res: u64 = 2;
        let d: u8 = 0;
        while (d < significant_digits) : (d += 1) {
            largest_single_unit_res *= 10;
        }

        let sub_mag: u6 = 1;
        let pow2: u64 = 2;
        while (pow2 < largest_single_unit_res) {
            pow2 <<= 1;
            sub_mag += 1;
        }

        if ((@as(u8, unit_magnitude) + @as(u8, sub_mag)) > 63) {
            return HistogramError.InvalidRange;
        }

        const sub_half_mag: u6 = sub_mag - 1;
        const sub_count: usize = @as(usize, 1) << sub_mag;
        const sub_half_count: usize = sub_count >> 1;
        const sub_mask: u64 = (@as(u64, sub_count - 1)) << unit_magnitude;
        const lzc_base: u7 = @as(u7, 64) - @as(u7, unit_magnitude) - @as(u7, sub_mag);

        let smallest_untrackable: u128 = (@as(u128, sub_count)) << unit_magnitude;
        let buckets_needed: usize = 1;
        const high_u128: u128 = @as(u128, highest_trackable_value);
        while (smallest_untrackable <= high_u128) {
            smallest_untrackable <<= 1;
            buckets_needed += 1;
        }

        const counts_len: usize = (buckets_needed + 1) * sub_half_count;
        const counts = alloc.alloc(u64, counts_len) catch return HistogramError.OutOfMemory;
        @memset(counts, 0);

        return HdrHistogram{
            alloc: alloc,
            lowest_discernible_value: lowest_discernible_value,
            highest_trackable_value: highest_trackable_value,
            significant_digits: significant_digits,
            unit_magnitude: unit_magnitude,
            sub_bucket_count_magnitude: sub_mag,
            sub_bucket_half_count_magnitude: sub_half_mag,
            sub_bucket_count: sub_count,
            sub_bucket_half_count: sub_half_count,
            sub_bucket_mask: sub_mask,
            leading_zero_count_base: lzc_base,
            bucket_count: buckets_needed,
            counts: counts,
            total_count: 0,
            min_value: std.math.maxInt(u64),
            max_value: 0,
        };
    }

    /// Release backing bucket array.
    pub fn deinit(*_) {
        if (_.counts.len > 0) {
            _.alloc.free(_.counts);
            _.counts = [];
        }
    }

    /// Clear all recorded samples while retaining allocated capacity.
    pub fn reset(*_) {
        @memset(_.counts, 0);
        _.total_count = 0;
        _.min_value = std.math.maxInt(u64);
        _.max_value = 0;
    }

    @inline fn countsIndexFor(value: u64): usize {
        const masked = value | _.sub_bucket_mask;
        const lzc: u7 = @as(u7, @clz(masked));
        const bucket_idx: usize = @as(usize, _.leading_zero_count_base - lzc);
        const shift: u6 = @as(u6, @intCast(bucket_idx)) + _.unit_magnitude;
        const sub_idx: usize = @as(usize, value >> shift);
        if (bucket_idx == 0) {
            return sub_idx;
        }
        return ((bucket_idx + 1) * _.sub_bucket_half_count) + (sub_idx - _.sub_bucket_half_count);
    }

    @inline fn valueFromIndex(idx: usize): u64 {
        let bucket_idx: usize = 0;
        let sub_idx: usize = idx;
        if (idx >= _.sub_bucket_count) {
            bucket_idx = (idx >> _.sub_bucket_half_count_magnitude) - 1;
            const mask = _.sub_bucket_half_count - 1;
            sub_idx = (idx & mask) + _.sub_bucket_half_count;
        }
        const shift: u6 = @as(u6, @intCast(bucket_idx)) + _.unit_magnitude;
        return (@as(u64, sub_idx)) << shift;
    }

    @inline fn unitSizeForIndex(idx: usize): u64 {
        let bucket_idx: usize = 0;
        if (idx >= _.sub_bucket_count) {
            bucket_idx = (idx >> _.sub_bucket_half_count_magnitude) - 1;
        }
        const shift: u6 = @as(u6, @intCast(bucket_idx)) + _.unit_magnitude;
        return @as(u64, 1) << shift;
    }

    /// Record a single sample value. Returns `ValueOutOfRange` if `value > highest_trackable_value`.
    pub fn recordValue(*_, value: u64): void ! HistogramError {
        return _.recordValues(value, 1);
    }

    /// Record `count` occurrences of `value`.
    pub fn recordValues(*_, value: u64, count: u64): void ! HistogramError {
        if (count == 0) {
            return;
        }
        if (value > _.highest_trackable_value) {
            return HistogramError.ValueOutOfRange;
        }
        const idx = _.countsIndexFor(value);
        _.counts[idx] += count;
        _.total_count += count;
        if (value < _.min_value) {
            _.min_value = value;
        }
        if (value > _.max_value) {
            _.max_value = value;
        }
    }

    /// Record a sample value while compensating for Coordinated Omission given `expected_interval`.
    /// When `value > expected_interval > 0`, synthetic stalled samples are backfilled at
    /// `value - k * expected_interval` for all `k >= 1` while `>= expected_interval`.
    pub fn recordValueWithExpectedInterval(*_, value: u64, expected_interval: u64): void ! HistogramError {
        _.recordValue(value)?;
        if (expected_interval == 0 || value <= expected_interval) {
            return;
        }
        let missing = value - expected_interval;
        while (missing >= expected_interval) {
            _.recordValue(missing)?;
            missing -= expected_interval;
        }
    }

    /// Losslessly merge another histogram with identical range and precision into `this`.
    pub fn merge(*_, other: HdrHistogram): void ! HistogramError {
        if (_.lowest_discernible_value != other.lowest_discernible_value ||
            _.highest_trackable_value != other.highest_trackable_value ||
            _.significant_digits != other.significant_digits ||
            _.counts.len != other.counts.len) {
            return HistogramError.IncompatibleHistogram;
        }
        let i: usize = 0;
        while (i < _.counts.len) : (i += 1) {
            const c = other.counts[i];
            if (c > 0) {
                _.counts[i] += c;
            }
        }
        _.total_count += other.total_count;
        if (other.total_count > 0) {
            if (other.min_value < _.min_value) {
                _.min_value = other.min_value;
            }
            if (other.max_value > _.max_value) {
                _.max_value = other.max_value;
            }
        }
    }

    /// Return total number of recorded samples.
    pub fn totalCount(): u64 {
        return _.total_count;
    }

    /// Return minimum recorded sample value (0 if empty).
    pub fn minValue(): u64 {
        if (_.total_count == 0) {
            return 0;
        }
        return _.min_value;
    }

    /// Return maximum recorded sample value (0 if empty).
    pub fn maxValue(): u64 {
        if (_.total_count == 0) {
            return 0;
        }
        return _.max_value;
    }

    /// Return arithmetic mean of recorded values using bucket midpoint representatives.
    pub fn mean(): f64 {
        if (_.total_count == 0) {
            return 0.0;
        }
        let sum_val: f64 = 0.0;
        let i: usize = 0;
        while (i < _.counts.len) : (i += 1) {
            const c = _.counts[i];
            if (c > 0) {
                const v_from = _.valueFromIndex(i);
                const unit = _.unitSizeForIndex(i);
                const mid = (v_from as f64) + ((unit as f64) * 0.5) - 0.5;
                sum_val += (c as f64) * mid;
            }
        }
        return sum_val / (_.total_count as f64);
    }

    /// Return population standard deviation of recorded values.
    pub fn stddev(): f64 {
        if (_.total_count == 0) {
            return 0.0;
        }
        const m = _.mean();
        let dev_sum: f64 = 0.0;
        let i: usize = 0;
        while (i < _.counts.len) : (i += 1) {
            const c = _.counts[i];
            if (c > 0) {
                const v_from = _.valueFromIndex(i);
                const unit = _.unitSizeForIndex(i);
                const mid = (v_from as f64) + ((unit as f64) * 0.5) - 0.5;
                const diff = mid - m;
                dev_sum += (c as f64) * diff * diff;
            }
        }
        return @sqrt(dev_sum / (_.total_count as f64));
    }

    /// Return the value at percentile `p` in `[0.0, 100.0]` (e.g., 50.0, 95.0, 99.0, 99.9, 99.99).
    pub fn valueAtPercentile(p: f64): u64 {
        if (_.total_count == 0) {
            return 0;
        }
        let pct = p;
        if (pct < 0.0) {
            pct = 0.0;
        }
        if (pct > 100.0) {
            pct = 100.0;
        }
        if (pct == 0.0) {
            return _.min_value;
        }

        const requested_f = @ceil((pct / 100.0) * (_.total_count as f64));
        let count_at_pct: u64 = requested_f as u64;
        if (count_at_pct < 1) {
            count_at_pct = 1;
        }

        let running: u64 = 0;
        let i: usize = 0;
        while (i < _.counts.len) : (i += 1) {
            const c = _.counts[i];
            if (c > 0) {
                running += c;
                if (running >= count_at_pct) {
                    const v_from = _.valueFromIndex(i);
                    const unit = _.unitSizeForIndex(i);
                    const high_eq = v_from + unit - 1;
                    if (high_eq > _.max_value && pct >= 100.0) {
                        return _.max_value;
                    }
                    return high_eq;
                }
            }
        }
        return _.max_value;
    }

    /// Return the percentage `[0.0, 100.0]` of recorded samples at or below `value`.
    pub fn percentileAtOrBelow(value: u64): f64 {
        if (_.total_count == 0) {
            return 0.0;
        }
        if (value >= _.highest_trackable_value) {
            return 100.0;
        }
        const target_idx = _.countsIndexFor(value);
        let le_count: u64 = 0;
        let i: usize = 0;
        while (i <= target_idx && i < _.counts.len) : (i += 1) {
            le_count += _.counts[i];
        }
        return ((le_count as f64) * 100.0) / (_.total_count as f64);
    }

    /// Return an iterator over all recorded non-zero buckets in ascending order.
    pub fn iterRecorded(): RecordedIter {
        return RecordedIter{
            counts: _.counts,
            sub_bucket_count: _.sub_bucket_count,
            sub_bucket_half_count: _.sub_bucket_half_count,
            sub_bucket_half_count_magnitude: _.sub_bucket_half_count_magnitude,
            unit_magnitude: _.unit_magnitude,
            total_count: _.total_count,
            pos: 0,
            cumulative_count: 0,
        };
    }

    /// Return approximate total sum of all recorded values (used for Prometheus `_sum` export).
    pub fn totalSumApprox(): f64 {
        if (_.total_count == 0) {
            return 0.0;
        }
        let sum_val: f64 = 0.0;
        let i: usize = 0;
        while (i < _.counts.len) : (i += 1) {
            const c = _.counts[i];
            if (c > 0) {
                const v_from = _.valueFromIndex(i);
                const unit = _.unitSizeForIndex(i);
                const mid = (v_from as f64) + ((unit as f64) * 0.5) - 0.5;
                sum_val += (c as f64) * mid;
            }
        }
        return sum_val;
    }

    /// Count how many recorded samples fall at or below `bound`.
    pub fn countAtOrBelow(bound: u64): u64 {
        if (_.total_count == 0) {
            return 0;
        }
        if (bound >= _.highest_trackable_value) {
            return _.total_count;
        }
        const target_idx = _.countsIndexFor(bound);
        let le_count: u64 = 0;
        let i: usize = 0;
        while (i <= target_idx && i < _.counts.len) : (i += 1) {
            le_count += _.counts[i];
        }
        return le_count;
    }

    /// Format a human-readable summary string containing count, min, max, mean, stddev,
    /// and P50/P95/P99/P99.9/P99.99 percentiles. Caller owns returned slice.
    pub fn formatSummary(alloc: std.mem.Allocator): []u8 ! HistogramError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        const s = std.fmt.allocPrint(
            alloc,
            "count={d} min={d} max={d} mean={d:.2} stddev={d:.2} p50={d} p95={d} p99={d} p99.9={d} p99.99={d}",
            .{
                _.totalCount(),
                _.minValue(),
                _.maxValue(),
                _.mean(),
                _.stddev(),
                _.valueAtPercentile(50.0),
                _.valueAtPercentile(95.0),
                _.valueAtPercentile(99.0),
                _.valueAtPercentile(99.9),
                _.valueAtPercentile(99.99),
            },
        ) catch return HistogramError.OutOfMemory;
        defer alloc.free(s);
        out.appendSlice(alloc, s) catch return HistogramError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return HistogramError.OutOfMemory;
    }

    /// Format Prometheus histogram exposition lines for the given `metric_name` and `le_bounds`.
    /// Caller owns returned slice.
    pub fn formatPrometheusBuckets(
        alloc: std.mem.Allocator,
        metric_name: []const u8,
        le_bounds: []const u64,
    ): []u8 ! HistogramError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        for (le of le_bounds) {
            const c = _.countAtOrBelow(le);
            const line = std.fmt.allocPrint(
                alloc,
                "{s}_bucket{{le=\"{d}\"}} {d}\n",
                .{ metric_name, le, c },
            ) catch return HistogramError.OutOfMemory;
            defer alloc.free(line);
            out.appendSlice(alloc, line) catch return HistogramError.OutOfMemory;
        }

        const tail = std.fmt.allocPrint(
            alloc,
            "{s}_bucket{{le=\"+Inf\"}} {d}\n{s}_sum {d:.2}\n{s}_count {d}\n",
            .{ metric_name, _.totalCount(), metric_name, _.totalSumApprox(), metric_name, _.totalCount() },
        ) catch return HistogramError.OutOfMemory;
        defer alloc.free(tail);
        out.appendSlice(alloc, tail) catch return HistogramError.OutOfMemory;

        return out.toOwnedSlice(alloc) catch return HistogramError.OutOfMemory;
    }
}
