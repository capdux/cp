// Type definitions for CP

export class hdr_histogram {
}

export fn hdr_init(lowest_discernible_value: i64, highest_trackable_value: i64, significant_figures: i32, result: **hdr_histogram!?!): i32;
export fn hdr_close(h: *hdr_histogram!);
export fn hdr_reset(h: *hdr_histogram!);
export fn hdr_get_memory_size(h: *hdr_histogram!): usize;
export fn hdr_record_value(h: *hdr_histogram!, value: i64): bool;
export fn hdr_record_values(h: *hdr_histogram!, value: i64, count: i64): bool;
export fn hdr_record_corrected_value(h: *hdr_histogram!, value: i64, expected_interval: i64): bool;
export fn hdr_min(h: *&hdr_histogram!): i64;
export fn hdr_max(h: *&hdr_histogram!): i64;
export fn hdr_mean(h: *&hdr_histogram!): f64;
export fn hdr_stddev(h: *&hdr_histogram!): f64;
export fn hdr_value_at_percentile(h: *&hdr_histogram!, percentile: f64): i64;
export error HdrError { InitFailed, InvalidHandle, ValueOutOfRange }

export class Histogram {
    handle: *hdr_histogram!?,
    fn init(_: auto, lowest: i64, highest: i64, sigfigs: i32): !void;
    fn record(value: i64): !void;
    fn recordCount(value: i64, count: i64): !void;
    fn recordCorrected(value: i64, expected_interval: i64): !void;
    fn reset(): !void;
    fn memorySize(): !usize;
    fn min(): !i64;
    fn max(): !i64;
    fn mean(): !f64;
    fn stddev(): !f64;
    fn percentile(p: f64): !i64;
    fn __cp_deinit(_: &mut Histogram);
}

