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
export fn hdr_record_corrected_values(h: *hdr_histogram!, value: i64, count: i64, expected_interval: i64): bool;
export fn hdr_add(h: *hdr_histogram!, from: *&hdr_histogram!): i64;
export fn hdr_add_while_correcting_for_coordinated_omission(h: *hdr_histogram!, from: *hdr_histogram!, expected_interval: i64): i64;
export fn hdr_min(h: *&hdr_histogram!): i64;
export fn hdr_max(h: *&hdr_histogram!): i64;
export fn hdr_mean(h: *&hdr_histogram!): f64;
export fn hdr_stddev(h: *&hdr_histogram!): f64;
export fn hdr_value_at_percentile(h: *&hdr_histogram!, percentile: f64): i64;
export fn hdr_count_at_value(h: *&hdr_histogram!, value: i64): i64;
export fn hdr_values_are_equivalent(h: *&hdr_histogram!, a: i64, b: i64): bool;
export fn hdr_lowest_equivalent_value(h: *&hdr_histogram!, value: i64): i64;
export fn hdr_median_equivalent_value(h: *&hdr_histogram!, value: i64): i64;
export fn hdr_next_non_equivalent_value(h: *&hdr_histogram!, value: i64): i64;
export fn hdr_size_of_equivalent_value_range(h: *&hdr_histogram!, value: i64): i64;
export error HdrError { InitFailed, InvalidHandle, ValueOutOfRange }

export fn hdrClampF64(v: f64, lo: f64, hi: f64): f64;
export fn hdrTotalCount(h: *&hdr_histogram!): i64;
export class Histogram {
    handle: *hdr_histogram!?,
    lowest_trackable: i64,
    highest_trackable: i64,
    sigfigs: i32,
    fn init(_: auto, lowest: i64, highest: i64, sigfigs: i32): !void;
    fn open(_: auto, lowest: i64, highest: i64, sigfigs: i32): !void;
    fn record(value: i64): !void;
    fn recordCount(value: i64, count: i64): !void;
    fn recordN(value: i64, count: i64): !void;
    fn recordCorrected(value: i64, expected_interval: i64): !void;
    fn recordCorrecting(value: i64, expected_interval: i64): !void;
    fn recordCorrectingN(value: i64, count: i64, expected_interval: i64): !void;
    fn saturatingRecord(value: i64): !void;
    fn reset(): !void;
    fn memorySize(): !usize;
    fn min(): !i64;
    fn max(): !i64;
    fn mean(): !f64;
    fn stddev(): !f64;
    fn stdev(): !f64;
    fn percentile(p: f64): !i64;
    fn valueAtPercentile(p: f64): !i64;
    fn valueAtQuantile(q: f64): !i64;
    fn len(): !u64;
    fn isEmpty(): !bool;
    fn percentileBelow(value: i64): !f64;
    fn quantileBelow(value: i64): !f64;
    fn add(other: Histogram): !i64;
    fn addCorrecting(other: Histogram, expected_interval: i64): !i64;
    fn countAt(value: i64): !i64;
    fn equivalent(a: i64, b: i64): !bool;
    fn lowestEquivalent(value: i64): !i64;
    fn medianEquivalent(value: i64): !i64;
    fn nextNonEquivalent(value: i64): !i64;
    fn sizeOfEquivalentRange(value: i64): !i64;
    fn __cp_deinit(_: &mut Histogram);
}

