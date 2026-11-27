// Type definitions for CP

export fn ddskZeroBuckets(): [DDSK_BUCKETS]u64;
export fn ddskClampAlpha(alpha: f64): f64;
export fn ddskAddKeyCount(sk: &mut DDSketch, key: i32, cnt: u64);
export class DDSketch {
    buckets: [DDSK_BUCKETS]u64,
    alpha: f64,
    gamma: f64,
    log_gamma: f64,
    min_index: i32,
    has_anchor: bool,
    zero_count: u64,
    total_count: u64,
    min_val: f64,
    max_val: f64,
    sum_val: f64,
    fn new(relative_accuracy: f64): DDSketch;
    fn add(_: auto, v: f64);
    fn count(): u64;
    fn sum(): f64;
    fn min(): f64;
    fn max(): f64;
    fn quantile(q: f64): f64;
    fn merge(_: auto, other: DDSketch);
}

