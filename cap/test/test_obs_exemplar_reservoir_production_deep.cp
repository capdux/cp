// Production Deep Test Suite for cap:obs Exemplar Reservoir (lib/obs/exemplar_reservoir.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    EXEMPLAR_MAX_RESERVOIR_SLOTS,
    EXEMPLAR_FLAG_SAMPLED,
    ExemplarRecord,
    formatExemplarOpenMetrics,
    AlignedHistogramExemplarReservoir,
} from "cap:obs";

fn approxEqF64(a: f64, b: f64, eps: f64): bool {
    const d: f64 = if (a >= b) (a - b) else (b - a);
    return d <= eps;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: AlignedHistogramBucket binary-search alignment & P99 tail retention
// ============================================================================
test("exemplar_reservoir: AlignedHistogramBucket binary-search alignment & P99 tail retention", fn() {
    const bounds: [4]f64 = [0.005, 0.05, 0.5, 5.0];
    let res = AlignedHistogramExemplarReservoir.initAligned(bounds[0..], true) ?? return;

    // Verify exact boundary placement (value <= bound[i])
    expect_eq(res.findBucketIndex(0.001), 0 as usize);
    expect_eq(res.findBucketIndex(0.005), 0 as usize);
    expect_eq(res.findBucketIndex(0.05), 1 as usize);
    expect_eq(res.findBucketIndex(0.5), 2 as usize);
    expect_eq(res.findBucketIndex(5.0), 3 as usize);
    expect_eq(res.findBucketIndex(12.5), 4 as usize); // +Inf overflow bucket

    // 100 fast requests in bucket 0 + 1 rare P99 slow request in +Inf bucket (bucket 4)
    let i: u64 = 0;
    while (i < 100) : (i += 1) {
        expect_true(res.observe(0.002, 1_000 + i, 1, 100 + i, 500 + i, EXEMPLAR_FLAG_SAMPLED));
    }
    // Single slow P99 outlier in +Inf bucket MUST be retained independently!
    expect_true(res.observe(8.75, 2_000, 0xAAAA, 0xBBBB, 0xCCCC, EXEMPLAR_FLAG_SAMPLED));

    expect_eq(res.stats().occupied_exemplars, 2 as usize);
    let out: [8]ExemplarRecord = [ExemplarRecord{}; 8];
    let out_s: []ExemplarRecord = out[0..];
    const n = res.collectAndReset(*out_s);
    expect_eq(n, 2 as usize);
    // Bucket 0 kept the latest fast trace (i = 99 -> trace_id_lo = 199)
    expect_eq(out[0].bucket_idx, 0 as u16);
    expect_eq(out[0].trace_id_lo, 199 as u64);
    // Bucket 4 (+Inf) retained the P99 slow outlier trace!
    expect_eq(out[1].bucket_idx, 4 as u16);
    expect_eq(out[1].trace_id_hi, 0xAAAA as u64);
    expect_eq(out[1].span_id, 0xCCCC as u64);
    expect_true(approxEqF64(out[1].value, 8.75, 1.0e-9));
});

// ============================================================================
// Test 2: SimpleFixedSize deterministic SplitMix64 Algorithm R sampling
// ============================================================================
test("exemplar_reservoir: SimpleFixedSize deterministic SplitMix64 Algorithm R sampling", fn() {
    const bounds: [2]f64 = [0.1, 1.0];
    let r1 = AlignedHistogramExemplarReservoir.initSimpleFixed(bounds[0..], 4, 123456789, true) ?? return;
    let r2 = AlignedHistogramExemplarReservoir.initSimpleFixed(bounds[0..], 4, 123456789, true) ?? return;

    let i: u64 = 1;
    while (i <= 200) : (i += 1) {
        expect_true(r1.observe((i as f64) * 0.01, i * 1000, 1, i, i + 100, EXEMPLAR_FLAG_SAMPLED));
        expect_true(r2.observe((i as f64) * 0.01, i * 1000, 1, i, i + 100, EXEMPLAR_FLAG_SAMPLED));
    }

    expect_eq(r1.stats().occupied_exemplars, 4 as usize);
    expect_eq(r1.stats().total_observations, 200 as u64);

    let out1: [4]ExemplarRecord = [ExemplarRecord{}; 4];
    let out2: [4]ExemplarRecord = [ExemplarRecord{}; 4];
    let out1_s: []ExemplarRecord = out1[0..];
    let out2_s: []ExemplarRecord = out2[0..];
    const n1 = r1.collectAndReset(*out1_s);
    const n2 = r2.collectAndReset(*out2_s);
    expect_eq(n1, 4 as usize);
    expect_eq(n2, 4 as usize);

    // Same seed -> 100% deterministic identical reservoir contents
    let k: usize = 0;
    while (k < 4) : (k += 1) {
        expect_eq(out1[k].trace_id_lo, out2[k].trace_id_lo);
        expect_eq(out1[k].span_id, out2[k].span_id);
    }
});

// ============================================================================
// Test 3: Unsampled & zero trace/span filtering while keeping histogram counts
// ============================================================================
test("exemplar_reservoir: unsampled & zero trace/span filtering preserves histogram counts", fn() {
    const bounds: [2]f64 = [0.1, 1.0];
    let res = AlignedHistogramExemplarReservoir.initAligned(bounds[0..], true) ?? return;

    // Unsampled flag (0x00) -> counted in histogram, filtered from exemplar
    expect_true(res.observe(0.05, 100, 1, 2, 3, 0x00));
    // Zero trace_id (hi=0, lo=0) -> counted in histogram, filtered from exemplar
    expect_true(res.observe(0.06, 101, 0, 0, 3, EXEMPLAR_FLAG_SAMPLED));
    // Zero span_id -> counted in histogram, filtered from exemplar
    expect_true(res.observe(0.07, 102, 1, 2, 0, EXEMPLAR_FLAG_SAMPLED));

    const st = res.stats();
    expect_eq(st.total_observations, 3 as u64);
    expect_eq(st.filtered_unsampled, 3 as u64);
    expect_eq(st.occupied_exemplars, 0 as usize);

    if (b0 := res.getBucketSummary(0)) {
        expect_eq(b0.count, 3 as u64);
        expect_true(approxEqF64(b0.sum, 0.18, 1.0e-9));
        expect_true(!b0.has_exemplar);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 4: Time-window eviction (evictOlderThan) & collectAndReset cycle
// ============================================================================
test("exemplar_reservoir: time-window evictOlderThan & collectAndReset scrape cycle", fn() {
    const bounds: [3]f64 = [0.1, 0.5, 1.0];
    let res = AlignedHistogramExemplarReservoir.initAligned(bounds[0..], true) ?? return;

    // 3 exemplars at timestamps 1000, 2000, 3000 in distinct buckets
    expect_true(res.observe(0.05, 1000, 1, 10, 100, EXEMPLAR_FLAG_SAMPLED));
    expect_true(res.observe(0.25, 2000, 1, 20, 200, EXEMPLAR_FLAG_SAMPLED));
    expect_true(res.observe(0.80, 3000, 1, 30, 300, EXEMPLAR_FLAG_SAMPLED));
    expect_eq(res.stats().occupied_exemplars, 3 as usize);

    // Evict older than cutoff_ns = 2500 -> evicts buckets 0 & 1 (timestamps 1000, 2000)
    const evicted = res.evictOlderThan(2500);
    expect_eq(evicted, 2 as usize);
    expect_eq(res.stats().evicted_expired, 2 as u64);
    expect_eq(res.stats().occupied_exemplars, 1 as usize);

    // Collect remaining exemplar & verify reservoir resets while histogram counts stay
    let out: [4]ExemplarRecord = [ExemplarRecord{}; 4];
    let out_s: []ExemplarRecord = out[0..];
    const n = res.collectAndReset(*out_s);
    expect_eq(n, 1 as usize);
    expect_eq(out[0].trace_id_lo, 30 as u64);
    expect_eq(res.stats().occupied_exemplars, 0 as usize);
    expect_eq(res.stats().total_observations, 3 as u64);

    // Second collect yields 0
    expect_eq(res.collectAndReset(*out_s), 0 as usize);
});

// ============================================================================
// Test 5: OpenMetrics 1.0 text formatting & buffer overflow guard
// ============================================================================
test("exemplar_reservoir: OpenMetrics 1.0 text formatting & buffer overflow guard", fn() {
    const ex = ExemplarRecord{
        value: 0.425,
        timestamp_ns: 1_700_000_001_250_000_000,
        trace_id_hi: 0x0123456789abcdef,
        trace_id_lo: 0xfedcba9876543210,
        span_id: 0x0011223344556677,
        sampled_flags: EXEMPLAR_FLAG_SAMPLED,
        bucket_idx: 2,
        occupied: true,
    };

    let buf: [128]u8 = [0; 128];
    let buf_s: []u8 = buf[0..];
    if (written := formatExemplarOpenMetrics(ex, *buf_s)) {
        const expected = "# {trace_id=\"0123456789abcdeffedcba9876543210\",span_id=\"0011223344556677\"} 0.425 1700000001.250";
        expect_true(bytesEq(buf[0..written], expected));
    } else {
        expect_true(false);
    }

    // Undersized buffer returns null safely
    let tiny: [32]u8 = [0; 32];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatExemplarOpenMetrics(ex, *tiny_s) == null);

    // Unoccupied exemplar returns null
    const empty_ex = ExemplarRecord{};
    expect_true(formatExemplarOpenMetrics(empty_ex, *buf_s) == null);
});

// ============================================================================
// Test 6: Per-bucket summary & small collect output buffer guard
// ============================================================================
test("exemplar_reservoir: per-bucket summary & small collect buffer guard", fn() {
    const bounds: [2]f64 = [1.0, 5.0];
    let res = AlignedHistogramExemplarReservoir.initAligned(bounds[0..], false) ?? return;

    expect_true(res.observe(0.5, 10, 1, 1, 1, 0));
    expect_true(res.observe(2.5, 11, 1, 2, 2, 0));
    expect_true(res.observe(9.0, 12, 1, 3, 3, 0));

    if (b_inf := res.getBucketSummary(2)) {
        expect_true(b_inf.is_inf);
        expect_eq(b_inf.count, 1 as u64);
        expect_true(approxEqF64(b_inf.sum, 9.0, 1.0e-9));
        expect_true(b_inf.has_exemplar);
    } else {
        expect_true(false);
    }
    expect_true(res.getBucketSummary(3) == null);

    // Small output buffer (len=1) safely writes 1 without overflow and resets reservoir
    let small_out: [1]ExemplarRecord = [ExemplarRecord{}; 1];
    let small_s: []ExemplarRecord = small_out[0..];
    expect_eq(res.collectAndReset(*small_s), 1 as usize);
    expect_eq(res.stats().occupied_exemplars, 0 as usize);
});

// ============================================================================
// Test 7: Strict boundary guards (non-monotonic bounds, NaN/Inf, invalid size)
// ============================================================================
test("exemplar_reservoir: strict boundary guards (bounds, NaN/Inf, invalid reservoir size)", fn() {
    const empty_b: [0]f64 = [];
    const non_mono: [3]f64 = [0.1, 0.5, 0.5]; // duplicate bound -> not strictly increasing
    const dec_b: [2]f64 = [1.0, 0.5];
    const nan_b: [2]f64 = [0.1, 0.0 / 0.0];

    expect_true(AlignedHistogramExemplarReservoir.initAligned(empty_b[0..], true) == null);
    expect_true(AlignedHistogramExemplarReservoir.initAligned(non_mono[0..], true) == null);
    expect_true(AlignedHistogramExemplarReservoir.initAligned(dec_b[0..], true) == null);
    expect_true(AlignedHistogramExemplarReservoir.initAligned(nan_b[0..], true) == null);

    const good_b: [2]f64 = [0.1, 1.0];
    expect_true(AlignedHistogramExemplarReservoir.initSimpleFixed(good_b[0..], 0, 1, true) == null);
    expect_true(AlignedHistogramExemplarReservoir.initSimpleFixed(good_b[0..], EXEMPLAR_MAX_RESERVOIR_SLOTS + 1, 1, true) == null);

    let res = AlignedHistogramExemplarReservoir.initAligned(good_b[0..], true) ?? return;
    expect_true(!res.observe(0.0 / 0.0, 100, 1, 1, 1, EXEMPLAR_FLAG_SAMPLED));
    expect_true(!res.observe(1.0 / 0.0, 101, 1, 1, 1, EXEMPLAR_FLAG_SAMPLED));
    expect_true(!res.observe(-1.0 / 0.0, 102, 1, 1, 1, EXEMPLAR_FLAG_SAMPLED));
    expect_eq(res.stats().rejected_invalid, 3 as u64);
    expect_eq(res.stats().total_observations, 0 as u64);
});
