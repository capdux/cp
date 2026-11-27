// @cap/hdrhistogram — HdrHistogram_c C-FFI & RAII safe wrapper for CP
// License: Public Domain (CC0-1.0) / BSD-2-Clause (Upstream: HdrHistogram/HdrHistogram_c)

pub @opaque struct hdr_histogram {}

pub @c {
    fn hdr_init(
        lowest_discernible_value: i64,
        highest_trackable_value: i64,
        significant_figures: i32,
        result: *(*hdr_histogram?!)!
    ): i32;
    fn hdr_close(h: *hdr_histogram!);
    fn hdr_reset(h: *hdr_histogram!);
    fn hdr_get_memory_size(h: *hdr_histogram!): usize;
    fn hdr_record_value(h: *hdr_histogram!, value: i64): bool;
    fn hdr_record_values(h: *hdr_histogram!, value: i64, count: i64): bool;
    fn hdr_record_corrected_value(h: *hdr_histogram!, value: i64, expected_interval: i64): bool;
    fn hdr_record_corrected_values(
        h: *hdr_histogram!,
        value: i64,
        count: i64,
        expected_interval: i64
    ): bool;
    fn hdr_add(h: *hdr_histogram!, from: *const hdr_histogram!): i64;
    fn hdr_add_while_correcting_for_coordinated_omission(
        h: *hdr_histogram!,
        from: *hdr_histogram!,
        expected_interval: i64
    ): i64;
    fn hdr_min(h: *const hdr_histogram!): i64;
    fn hdr_max(h: *const hdr_histogram!): i64;
    fn hdr_mean(h: *const hdr_histogram!): f64;
    fn hdr_stddev(h: *const hdr_histogram!): f64;
    fn hdr_value_at_percentile(h: *const hdr_histogram!, percentile: f64): i64;
    fn hdr_count_at_value(h: *const hdr_histogram!, value: i64): i64;
    fn hdr_values_are_equivalent(h: *const hdr_histogram!, a: i64, b: i64): bool;
    fn hdr_lowest_equivalent_value(h: *const hdr_histogram!, value: i64): i64;
    fn hdr_median_equivalent_value(h: *const hdr_histogram!, value: i64): i64;
    fn hdr_next_non_equivalent_value(h: *const hdr_histogram!, value: i64): i64;
    fn hdr_size_of_equivalent_value_range(h: *const hdr_histogram!, value: i64): i64;
}

pub err HdrError {
    InitFailed,
    InvalidHandle,
    ValueOutOfRange,
}

fn hdrClampF64(v: f64, lo: f64, hi: f64): f64 {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

fn hdrTotalCount(h: *const hdr_histogram!): i64 {
    let total: i64 = hdr_count_at_value(h, 0);
    const min_v = hdr_min(h);
    const max_v = hdr_max(h);
    if (min_v <= max_v && max_v > 0) {
        let v = min_v;
        while (v <= max_v) {
            total += hdr_count_at_value(h, v);
            const next = hdr_next_non_equivalent_value(h, v);
            if (next <= v) break;
            v = next;
        }
    }
    return total;
}

/// High Dynamic Range latency histogram owning a native `hdr_histogram` with deterministic `~init()` RAII release.
pub class Histogram {
    pub handle: *hdr_histogram?! = null,
    pub lowest_trackable: i64 = 1,
    pub highest_trackable: i64 = 0,
    pub sigfigs: i32 = 3,

    pub fn init(*_, lowest: i64, highest: i64, sigfigs: i32): void! {
        let out: *hdr_histogram?! = null;
        if (hdr_init(lowest, highest, sigfigs, &out) != 0) {
            return HdrError.InitFailed;
        }
        _.handle = out orelse return HdrError.InitFailed;
        _.lowest_trackable = lowest;
        _.highest_trackable = highest;
        _.sigfigs = sigfigs;
    }

    /// Initializes the underlying native `hdr_histogram` instance with `[lowest, highest]` range and `sigfigs` precision (1..5).
    pub fn open(*_, lowest: i64, highest: i64, sigfigs: i32): void! {
        if (old := _.handle) {
            hdr_close(old);
            _.handle = null;
        }
        let out: *hdr_histogram?! = null;
        if (hdr_init(lowest, highest, sigfigs, &out) != 0) {
            return HdrError.InitFailed;
        }
        _.handle = out orelse return HdrError.InitFailed;
        _.lowest_trackable = lowest;
        _.highest_trackable = highest;
        _.sigfigs = sigfigs;
    }

    pub fn record(value: i64): void! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        if (!hdr_record_value(h, value)) return HdrError.ValueOutOfRange;
    }

    pub fn recordCount(value: i64, count: i64): void! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        if (!hdr_record_values(h, value, count)) return HdrError.ValueOutOfRange;
    }

    /// Crates `hdrhistogram` alias for recording `count` occurrences of `value`.
    pub fn recordN(value: i64, count: i64): void! {
        return _.recordCount(value, count);
    }

    pub fn recordCorrected(value: i64, expected_interval: i64): void! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        if (!hdr_record_corrected_value(h, value, expected_interval)) return HdrError.ValueOutOfRange;
    }

    /// Crates `hdrhistogram` alias for coordinated-omission corrected recording.
    pub fn recordCorrecting(value: i64, expected_interval: i64): void! {
        return _.recordCorrected(value, expected_interval);
    }

    /// Records `count` occurrences of `value` while correcting for coordinated omission.
    pub fn recordCorrectingN(value: i64, count: i64, expected_interval: i64): void! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        if (!hdr_record_corrected_values(h, value, count, expected_interval)) {
            return HdrError.ValueOutOfRange;
        }
    }

    /// Records `value`, clamping it into `[0, highest_trackable]` instead of failing on overflow.
    pub fn saturatingRecord(value: i64): void! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        let clamped = value;
        if (clamped < 0) clamped = 0;
        if (_.highest_trackable > 0 && clamped > _.highest_trackable) {
            clamped = _.highest_trackable;
        }
        if (!hdr_record_value(h, clamped)) return HdrError.ValueOutOfRange;
    }

    pub fn reset(): void! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        hdr_reset(h);
    }

    pub fn memorySize(): usize! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_get_memory_size(h);
    }

    pub fn min(): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_min(h);
    }

    pub fn max(): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_max(h);
    }

    pub fn mean(): f64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_mean(h);
    }

    pub fn stddev(): f64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_stddev(h);
    }

    /// Crates `hdrhistogram` alias for standard deviation (`stdev`).
    pub fn stdev(): f64! {
        return _.stddev();
    }

    pub fn percentile(p: f64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_value_at_percentile(h, p);
    }

    /// Returns the value at percentile `p` in `[0.0, 100.0]`.
    pub fn valueAtPercentile(p: f64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_value_at_percentile(h, hdrClampF64(p, 0.0, 100.0));
    }

    /// Returns the value at quantile `q` in `[0.0, 1.0]`.
    pub fn valueAtQuantile(q: f64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_value_at_percentile(h, hdrClampF64(q, 0.0, 1.0) * 100.0);
    }

    /// Returns total number of recorded samples in the histogram.
    pub fn len(): u64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        const n = hdrTotalCount(h);
        if (n < 0) return 0;
        return n as u64;
    }

    /// Returns `true` if no samples have been recorded.
    pub fn isEmpty(): bool! {
        const n = _.len()?;
        return n == 0;
    }

    /// Returns the percentage `[0.0, 100.0]` of recorded values that are `<= value`.
    pub fn percentileBelow(value: i64): f64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        const c0 = hdr_count_at_value(h, 0);
        let total: i64 = c0;
        let below: i64 = if (value >= 0) { c0 } else { 0 };

        const min_v = hdr_min(h);
        const max_v = hdr_max(h);
        if (min_v <= max_v && max_v > 0) {
            let v = min_v;
            while (v <= max_v) {
                const c = hdr_count_at_value(h, v);
                total += c;
                if (hdr_lowest_equivalent_value(h, v) <= value) {
                    below += c;
                }
                const next = hdr_next_non_equivalent_value(h, v);
                if (next <= v) break;
                v = next;
            }
        }
        if (total <= 0) return 0.0;
        return (below as f64) * 100.0 / (total as f64);
    }

    /// Returns the quantile `[0.0, 1.0]` of recorded values that are `<= value`.
    pub fn quantileBelow(value: i64): f64! {
        const pct = _.percentileBelow(value)?;
        return pct / 100.0;
    }

    /// Merges all recorded values from `other` into this histogram.
    /// Returns the number of dropped samples (values out of range).
    pub fn add(other: Histogram): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        const src = other.handle orelse return HdrError.InvalidHandle;
        return hdr_add(h, src);
    }

    /// Merges all values from `other` while correcting for coordinated omission.
    pub fn addCorrecting(other: Histogram, expected_interval: i64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        const src = other.handle orelse return HdrError.InvalidHandle;
        return hdr_add_while_correcting_for_coordinated_omission(h, src, expected_interval);
    }

    /// Returns the count of recorded samples equivalent to `value`.
    pub fn countAt(value: i64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_count_at_value(h, value);
    }

    /// Returns `true` if `a` and `b` map to the same bucket under this histogram's precision.
    pub fn equivalent(a: i64, b: i64): bool! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_values_are_equivalent(h, a, b);
    }

    pub fn lowestEquivalent(value: i64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_lowest_equivalent_value(h, value);
    }

    pub fn medianEquivalent(value: i64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_median_equivalent_value(h, value);
    }

    pub fn nextNonEquivalent(value: i64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_next_non_equivalent_value(h, value);
    }

    pub fn sizeOfEquivalentRange(value: i64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_size_of_equivalent_value_range(h, value);
    }

    ~init() {
        if (h := _.handle) {
            hdr_close(h);
            _.handle = null;
        }
    }
}
