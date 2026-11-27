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
    fn hdr_min(h: *const hdr_histogram!): i64;
    fn hdr_max(h: *const hdr_histogram!): i64;
    fn hdr_mean(h: *const hdr_histogram!): f64;
    fn hdr_stddev(h: *const hdr_histogram!): f64;
    fn hdr_value_at_percentile(h: *const hdr_histogram!, percentile: f64): i64;
}

pub err HdrError {
    InitFailed,
    InvalidHandle,
    ValueOutOfRange,
}

/// High Dynamic Range latency histogram owning a native `hdr_histogram` with deterministic `~init()` RAII release.
pub class Histogram {
    pub handle: *hdr_histogram?! = null,

    pub fn init(*_, lowest: i64, highest: i64, sigfigs: i32): void! {
        let out: *hdr_histogram?! = null;
        if (hdr_init(lowest, highest, sigfigs, &out) != 0) {
            return HdrError.InitFailed;
        }
        _.handle = out orelse return HdrError.InitFailed;
    }

    pub fn record(value: i64): void! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        if (!hdr_record_value(h, value)) return HdrError.ValueOutOfRange;
    }

    pub fn recordCount(value: i64, count: i64): void! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        if (!hdr_record_values(h, value, count)) return HdrError.ValueOutOfRange;
    }

    pub fn recordCorrected(value: i64, expected_interval: i64): void! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        if (!hdr_record_corrected_value(h, value, expected_interval)) return HdrError.ValueOutOfRange;
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

    pub fn percentile(p: f64): i64! {
        const h = _.handle orelse return HdrError.InvalidHandle;
        return hdr_value_at_percentile(h, p);
    }

    ~init() {
        if (h := _.handle) {
            hdr_close(h);
            _.handle = null;
        }
    }
}
