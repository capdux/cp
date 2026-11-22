// Origin: Pure-CP Stdlib Test Suite (lib/obs/hdr_histogram.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc HDR Log-Linear Latency Histogram:
//   1. Log-linear bucket monotonicity, exact [0..127] base band & <= 1.5625% relative error bound
//   2. Small-value exact quantiles & high-dynamic-range P50/P90/P95/P99/P99.9 accuracy
//   3. Bimodal/multimodal latency distribution & long-tail spike capture
//   4. Coordinated Omission correction via record_with_expected_interval
//   5. Lossless multi-worker merge_from equivalence & config mismatch rejection
//   6. Full reset & clean reuse preserving max_trackable_val
//   7. Boundary guards (empty histogram, invalid quantiles, clamping, zero count, max init clamp)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    HDR_SUB_BUCKETS,
    HDR_NUM_SCALES,
    HDR_TOTAL_BUCKETS,
    HDR_MAX_SUPPORTED_VALUE,
    HdrQuantileSummary,
    HdrHistogram,
} from "cap:obs";

fn approxEqF64(a: f64, b: f64, eps: f64): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: Bucket Mapping Monotonicity, Exact Base Band & Relative Precision Bound
// ============================================================================
test("hdr_histogram: log-linear bucket monotonicity & <= 1.5625% relative precision", () => {
    expect_eq(HDR_SUB_BUCKETS, 64);
    expect_eq(HDR_NUM_SCALES, 28);
    expect_eq(HDR_TOTAL_BUCKETS, 1920);

    // Exact 1:1 mapping for [0, 127]
    let v: u64 = 0;
    while (v < 128) : (v += 1) {
        const b = HdrHistogram.value_to_bucket(v);
        expect_eq(b, v as usize);
        expect_eq(HdrHistogram.bucket_upper_bound(b), v);
    }

    // Scale transitions & relative error <= 1/64 (1.5625%) across 10 orders of magnitude
    const probes: [12]u64 = [
        128,
        129,
        255,
        256,
        1_000,
        10_000,
        65_535,
        1_000_000,
        50_000_000,
        1_000_000_000,
        10_000_000_000,
        HDR_MAX_SUPPORTED_VALUE,
    ];

    let prev_bucket: usize = 127;
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        const val = probes[i];
        const b = HdrHistogram.value_to_bucket(val);
        const upper = HdrHistogram.bucket_upper_bound(b);

        expect_eq(b >= prev_bucket, true);
        expect_eq(upper >= val, true);

        const rel_err: f64 = ((upper - val) as f64) / (val as f64);
        expect_eq(rel_err <= (1.0 / 64.0) + 0.0000001, true);
        prev_bucket = b;
    }
});

// ============================================================================
// Test 2: Exact Small-Value Quantiles & High-Dynamic-Range Accuracy
// ============================================================================
test("hdr_histogram: uniform [1..1000] quantiles and high-dynamic-range summary", () => {
    let h = HdrHistogram.init(10_000_000);
    let x: u64 = 1;
    while (x <= 1000) : (x += 1) {
        expect_eq(h.record(x), true);
    }

    const s = h.summarize();
    expect_eq(s.total_count, 1000);
    expect_eq(s.min_val, 1);
    expect_eq(s.max_val, 1000);
    expect_eq(approxEqF64(s.mean_f64, 500.5, 0.001), true);

    // Within <= 1.6% relative bound of true quantiles (500, 900, 950, 990, 999)
    expect_eq(s.p50 >= 500 && s.p50 <= 508, true);
    expect_eq(s.p90 >= 900 && s.p90 <= 915, true);
    expect_eq(s.p95 >= 950 && s.p95 <= 966, true);
    expect_eq(s.p99 >= 990 && s.p99 <= 1000, true);
    expect_eq(s.p999 >= 999 && s.p999 <= 1000, true);
});

// ============================================================================
// Test 3: Bimodal / Multimodal Latency Distribution & Long-Tail Capture
// ============================================================================
test("hdr_histogram: captures bimodal/multimodal latency tail accurately", () => {
    let h = HdrHistogram.init(60_000_000);

    // 10,000 samples:
    //   9500 fast cache hits @ 500us
    //    400 DB queries      @ 20,000us (20ms)
    //     90 slow queries    @ 200,000us (200ms)
    //     10 GC pauses       @ 2,000,000us (2s)
    expect_eq(h.record_n(500, 9500), true);
    expect_eq(h.record_n(20_000, 400), true);
    expect_eq(h.record_n(200_000, 90), true);
    expect_eq(h.record_n(2_000_000, 10), true);

    const s = h.summarize();
    expect_eq(s.total_count, 10_000);
    expect_eq(s.min_val, 500);
    expect_eq(s.max_val, 2_000_000);

    // P50, P90, P95 stay in the 500us fast tier
    expect_eq(s.p50 >= 500 && s.p50 <= 512, true);
    expect_eq(s.p90 >= 500 && s.p90 <= 512, true);
    expect_eq(s.p95 >= 500 && s.p95 <= 512, true);
    // P99 lands in the 20,000us DB tier (rank 9900)
    expect_eq(s.p99 >= 20_000 && s.p99 <= 20_400, true);
    // P99.9 lands in the 200,000us slow tier (rank 9990)
    expect_eq(s.p999 >= 200_000 && s.p999 <= 204_000, true);
});

// ============================================================================
// Test 4: Coordinated Omission Correction (record_with_expected_interval)
// ============================================================================
test("hdr_histogram: Coordinated Omission correction exposes hidden stall latency", () => {
    let raw_h = HdrHistogram.init(10_000_000);
    let co_h = HdrHistogram.init(10_000_000);

    // 990 fast requests at 100us
    expect_eq(raw_h.record_n(100, 990), true);
    expect_eq(co_h.record_n(100, 990), true);

    // 1 stalled request taking 100,000us when expected arrival interval is 10,000us
    expect_eq(raw_h.record(100_000), true);
    expect_eq(co_h.record_with_expected_interval(100_000, 10_000), true);

    // Raw histogram hides the stall at P99 (only 1 sample out of 991 -> rank 982 is still 100us)
    expect_eq(raw_h.total_count, 991);
    expect_eq(raw_h.value_at_quantile(0.99), 100);

    // CO-corrected histogram backfills 9 synthetic stalled samples (90k, 80k, ..., 10k) -> total 1000
    expect_eq(co_h.total_count, 1000);
    // Rank 991+ are all >= 10,000us; rank 990 is 100us, rank 991 is 10,000us -> P99.1 >= 10,000us, P99.5 >= 50,000us
    expect_eq(co_h.value_at_quantile(0.991) >= 10_000, true);
    expect_eq(co_h.value_at_quantile(0.995) >= 50_000, true);
    expect_eq(co_h.value_at_quantile(0.999) >= 90_000, true);
});

// ============================================================================
// Test 5: Lossless Cross-Worker merge_from Equivalence & Config Guard
// ============================================================================
test("hdr_histogram: lossless multi-worker merge_from matches single reference", () => {
    let w1 = HdrHistogram.init(5_000_000);
    let w2 = HdrHistogram.init(5_000_000);
    let w3 = HdrHistogram.init(5_000_000);
    let ref_h = HdrHistogram.init(5_000_000);

    expect_eq(w1.record_n(40, 300), true);
    expect_eq(w1.record_n(1_500, 200), true);
    expect_eq(w2.record_n(8_000, 400), true);
    expect_eq(w3.record_n(250_000, 100), true);

    expect_eq(ref_h.record_n(40, 300), true);
    expect_eq(ref_h.record_n(1_500, 200), true);
    expect_eq(ref_h.record_n(8_000, 400), true);
    expect_eq(ref_h.record_n(250_000, 100), true);

    let merged = HdrHistogram.init(5_000_000);
    expect_eq(merged.merge_from(w1), true);
    expect_eq(merged.merge_from(w2), true);
    expect_eq(merged.merge_from(w3), true);

    const sm = merged.summarize();
    const sr = ref_h.summarize();
    expect_eq(sm.total_count, sr.total_count);
    expect_eq(sm.min_val, sr.min_val);
    expect_eq(sm.max_val, sr.max_val);
    expect_eq(approxEqF64(sm.mean_f64, sr.mean_f64, 0.0001), true);
    expect_eq(sm.p50, sr.p50);
    expect_eq(sm.p90, sr.p90);
    expect_eq(sm.p95, sr.p95);
    expect_eq(sm.p99, sr.p99);
    expect_eq(sm.p999, sr.p999);

    // Mismatched max_trackable_val rejected cleanly
    let other_cfg = HdrHistogram.init(1_000_000);
    expect_eq(other_cfg.record(500), true);
    expect_eq(merged.merge_from(other_cfg), false);
});

// ============================================================================
// Test 6: Full reset & Clean Reuse
// ============================================================================
test("hdr_histogram: reset clears all state while preserving max_trackable_val", () => {
    let h = HdrHistogram.init(10_000);
    expect_eq(h.record_n(500, 50), true);
    expect_eq(h.record(99_999), false); // clamped
    expect_eq(h.total_count, 51);
    expect_eq(h.clamped_count, 1);

    h.reset();
    expect_eq(h.max_trackable_val, 10_000);
    expect_eq(h.total_count, 0);
    expect_eq(h.clamped_count, 0);
    const s0 = h.summarize();
    expect_eq(s0.total_count, 0);
    expect_eq(s0.p50, 0);

    // Record fresh batch after reset
    expect_eq(h.record_n(42, 10), true);
    const s1 = h.summarize();
    expect_eq(s1.total_count, 10);
    expect_eq(s1.min_val, 42);
    expect_eq(s1.max_val, 42);
    expect_eq(s1.p50, 42);
    expect_eq(s1.p99, 42);
});

// ============================================================================
// Test 7: Boundary Guards (Empty, Invalid Quantiles, Clamping, Zero Count)
// ============================================================================
test("hdr_histogram: boundary guards for empty, invalid quantiles & value clamping", () => {
    // init(0) and init(u64_max) clamp safely to HDR_MAX_SUPPORTED_VALUE
    const h_zero_init = HdrHistogram.init(0);
    expect_eq(h_zero_init.max_trackable_val, HDR_MAX_SUPPORTED_VALUE);
    const h_max_init = HdrHistogram.init(0xFFFFFFFFFFFFFFFF);
    expect_eq(h_max_init.max_trackable_val, HDR_MAX_SUPPORTED_VALUE);

    let h = HdrHistogram.init(1_000);
    // Empty histogram returns 0 for all quantiles
    expect_eq(h.value_at_quantile(0.0), 0);
    expect_eq(h.value_at_quantile(0.5), 0);
    expect_eq(h.value_at_quantile(1.0), 0);

    // Zero count is a safe no-op
    expect_eq(h.record_n(500, 0), true);
    expect_eq(h.total_count, 0);

    // Clamped recording
    expect_eq(h.record(100), true);
    expect_eq(h.record(5_000), false);
    expect_eq(h.total_count, 2);
    expect_eq(h.clamped_count, 1);
    expect_eq(h.max_val, 1_000);
    expect_eq(h.value_at_quantile(0.0), 100);
    expect_eq(h.value_at_quantile(1.0), 1_000);

    // Invalid quantiles return 0
    const nan_f64: f64 = std.math.nan(f64);
    const inf_f64: f64 = std.math.inf(f64);
    expect_eq(h.value_at_quantile(-0.01), 0);
    expect_eq(h.value_at_quantile(1.01), 0);
    expect_eq(h.value_at_quantile(nan_f64), 0);
    expect_eq(h.value_at_quantile(inf_f64), 0);
});
