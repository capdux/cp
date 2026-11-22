// Origin: Pure-CP Stdlib Test Suite (lib/obs/telemetry_export.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Latency Histogram & OTLP Span Batch Exporter:
//   1. LatencyBucketHistogram observe, min/max/mean & p50/p95/p99 quantile estimation
//   2. Custom strictly-increasing bucket bounds & invalid-bounds rejection
//   3. Prometheus/OpenMetrics _bucket/_sum/_count text rendering
//   4. OtlpSpanBatch multi-span collection, OTLP/HTTP JSON export & RFC 8259 escaping
//   5. OtlpSpanBatch capacity overflow drop tracking, clear reuse, empty batch & invalid span guards
//   6. Empty histogram, invalid quantile & undersized buffer defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HISTOGRAM_BUCKET_COUNT,
    LatencyBucketHistogram,
    OtlpSpanRecord,
    OtlpSpanBatch,
} from "cap:obs";

fn containsSlice(haystack: string, needle: string): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

// ============================================================================
// Test 1: LatencyBucketHistogram Observe, Stats & p50/p95/p99 Quantiles
// ============================================================================

fn testHistogramObserveAndQuantiles()! {
    let hist = cap.obs.LatencyBucketHistogram.init();
    expect_eq(HISTOGRAM_BUCKET_COUNT, 16);
    expect_eq(hist.count(), 0);

    // 100 samples:
    // 50 fast samples in (100..250] us
    // 45 medium samples in (1000..2500] us
    // 4 slow samples in (50000..100000] us
    // 1 outlier in +Inf bucket (20_000_000 us = 20s)
    let i: usize = 0;
    while (i < 50) : (i += 1) {
        hist.observe(200);
    }
    i = 0;
    while (i < 45) : (i += 1) {
        hist.observe(2000);
    }
    i = 0;
    while (i < 4) : (i += 1) {
        hist.observe(80000);
    }
    hist.observe(20000000);

    expect_eq(hist.count(), 100);
    expect_eq(hist.minUs() ?? 0, 200);
    expect_eq(hist.maxUs() ?? 0, 20000000);
    expect_eq(hist.sumUs(), 50 * 200 + 45 * 2000 + 4 * 80000 + 20000000);

    // Quantile checks:
    // p50 lands at rank 50 -> upper edge of (100, 250] bucket => 250.0
    const p50 = hist.quantile(0.50) ?? 0.0;
    expect_eq(p50 >= 249.0 && p50 <= 251.0, true);

    // p95 lands at rank 95 -> upper edge of (1000, 2500] bucket => 2500.0
    const p95 = hist.quantile(0.95) ?? 0.0;
    expect_eq(p95 >= 2499.0 && p95 <= 2501.0, true);

    // p99 lands at rank 99 -> upper edge of (50000, 100000] bucket => 100000.0
    const p99 = hist.quantile(0.99) ?? 0.0;
    expect_eq(p99 >= 99999.0 && p99 <= 100001.0, true);

    // p99.5 lands in +Inf overflow bucket between 10_000_000 and 20_000_000 => ~15_000_000
    const p995 = hist.quantile(0.995) ?? 0.0;
    expect_eq(p995 > 14000000.0 && p995 < 16000000.0, true);

    // Reset clears everything
    hist.reset();
    expect_eq(hist.count(), 0);
    expect_eq(hist.minUs() == null, true);
}

// ============================================================================
// Test 2: Custom Bounds Validation
// ============================================================================

fn testCustomBoundsValidation()! {
    const valid_bounds: [16]u64 = [
        10, 20, 30, 40, 50, 60, 70, 80,
        90, 100, 200, 300, 400, 500, 600, 700,
    ];
    let h = LatencyBucketHistogram.initWithBounds(valid_bounds) ?? return error.InitFailed;
    h.observe(25);
    expect_eq(h.count(), 1);
    const p50 = h.quantile(0.5) ?? 0.0;
    // Bucket (20, 30], rank 0.5 -> 25.0
    expect_eq(p50 >= 24.9 && p50 <= 25.1, true);

    // Invalid: zero first bound
    const zero_bounds: [16]u64 = [
        0, 20, 30, 40, 50, 60, 70, 80,
        90, 100, 200, 300, 400, 500, 600, 700,
    ];
    expect_eq(LatencyBucketHistogram.initWithBounds(zero_bounds) == null, true);

    // Invalid: non-increasing bound
    const bad_bounds: [16]u64 = [
        10, 20, 20, 40, 50, 60, 70, 80,
        90, 100, 200, 300, 400, 500, 600, 700,
    ];
    expect_eq(LatencyBucketHistogram.initWithBounds(bad_bounds) == null, true);
}

// ============================================================================
// Test 3: Prometheus / OpenMetrics Text Exposition Rendering
// ============================================================================

fn testRenderPrometheusHistogram()! {
    let hist = LatencyBucketHistogram.init();
    hist.observe(40);     // <= 50
    hist.observe(600);    // <= 1000
    hist.observe(15000000); // +Inf

    let out_buf: [2048]u8 = [0; 2048];
    const n = hist.renderPrometheus("rpc_latency_us", out_buf[0..]) ?? return error.RenderFailed;
    const text = out_buf[0..n];

    expect_eq(containsSlice(text, "# TYPE rpc_latency_us histogram\n"), true);
    expect_eq(containsSlice(text, "rpc_latency_us_bucket{le=\"50\"} 1\n"), true);
    expect_eq(containsSlice(text, "rpc_latency_us_bucket{le=\"1000\"} 2\n"), true);
    expect_eq(containsSlice(text, "rpc_latency_us_bucket{le=\"10000000\"} 2\n"), true);
    expect_eq(containsSlice(text, "rpc_latency_us_bucket{le=\"+Inf\"} 3\n"), true);
    expect_eq(containsSlice(text, "rpc_latency_us_sum 15000640\n"), true);
    expect_eq(containsSlice(text, "rpc_latency_us_count 3\n"), true);
}

// ============================================================================
// Test 4: OtlpSpanBatch Multi-Span Collection & OTLP/HTTP JSON Export
// ============================================================================

fn testOtlpSpanBatchExportAndEscaping()! {
    let slots: [4]OtlpSpanRecord = [OtlpSpanRecord{}; 4];
    let batch = OtlpSpanBatch.init(slots[0..]);

    expect_eq(batch.push(OtlpSpanRecord{
        trace_id_hex: "0123456789abcdef0123456789abcdef",
        span_id_hex: "1111222233334444",
        parent_span_id_hex: "",
        name: "POST \"/v1/chat\"",
        start_us: 1000,
        end_us: 2500,
        status_code: 200,
        is_error: false,
    }), true);

    expect_eq(batch.push(OtlpSpanRecord{
        trace_id_hex: "0123456789abcdef0123456789abcdef",
        span_id_hex: "5555666677778888",
        parent_span_id_hex: "1111222233334444",
        name: "tool\ncall",
        start_us: 1200,
        end_us: 2000,
        status_code: 503,
        is_error: true,
    }), true);

    expect_eq(batch.len(), 2);

    let out_buf: [2048]u8 = [0; 2048];
    const n = batch.renderOtlpJson("cp-agent\"svc", out_buf[0..]) ?? return error.RenderFailed;
    const json = out_buf[0..n];

    expect_eq(containsSlice(json, "\"stringValue\":\"cp-agent\\\"svc\""), true);
    expect_eq(containsSlice(json, "\"name\":\"POST \\\"/v1/chat\\\"\""), true);
    expect_eq(containsSlice(json, "\"startTimeUnixNano\":\"1000000\""), true);
    expect_eq(containsSlice(json, "\"endTimeUnixNano\":\"2500000\""), true);
    expect_eq(containsSlice(json, "\"intValue\":\"200\""), true);
    expect_eq(containsSlice(json, "\"status\":{\"code\":1}"), true);
    expect_eq(containsSlice(json, "\"parentSpanId\":\"1111222233334444\""), true);
    expect_eq(containsSlice(json, "\"name\":\"tool\\ncall\""), true);
    expect_eq(containsSlice(json, "\"status\":{\"code\":2}"), true);
}

// ============================================================================
// Test 5: OtlpSpanBatch Capacity Drops, Clear Reuse & Empty Batch
// ============================================================================

fn testOtlpSpanBatchDropsClearAndEmpty()! {
    let slots: [1]OtlpSpanRecord = [OtlpSpanRecord{}; 1];
    let batch = OtlpSpanBatch.init(slots[0..]);

    // Empty batch renders valid empty spans array
    let out_buf: [512]u8 = [0; 512];
    const n_empty = batch.renderOtlpJson("svc", out_buf[0..]) ?? return error.RenderFailed;
    expect_eq(containsSlice(out_buf[0..n_empty], "\"spans\":[]"), true);

    // Invalid spans rejected (empty fields or end < start) without counting as capacity drop
    expect_eq(batch.push(OtlpSpanRecord{ trace_id_hex: "", span_id_hex: "1", name: "a" }), false);
    expect_eq(batch.push(OtlpSpanRecord{ trace_id_hex: "t", span_id_hex: "1", name: "a", start_us: 100, end_us: 50 }), false);
    expect_eq(batch.dropped(), 0);

    // Fill capacity (1), then overflow increments dropped()
    expect_eq(batch.push(OtlpSpanRecord{ trace_id_hex: "t1", span_id_hex: "s1", name: "op1", start_us: 10, end_us: 20 }), true);
    expect_eq(batch.push(OtlpSpanRecord{ trace_id_hex: "t2", span_id_hex: "s2", name: "op2", start_us: 20, end_us: 30 }), false);
    expect_eq(batch.dropped(), 1);
    expect_eq(batch.len(), 1);

    // Clear resets len to 0 and allows pushing again
    batch.clear();
    expect_eq(batch.len(), 0);
    expect_eq(batch.push(OtlpSpanRecord{ trace_id_hex: "t3", span_id_hex: "s3", name: "op3", start_us: 30, end_us: 40 }), true);
    expect_eq(batch.len(), 1);
}

// ============================================================================
// Test 6: Defensive Guards (Empty Histogram, Invalid q, Undersized Buffers)
// ============================================================================

fn testDefensiveGuards()! {
    let hist = LatencyBucketHistogram.init();
    const nan_f64: f64 = std.math.nan(f64);

    // Empty histogram stats return null
    expect_eq(hist.minUs() == null, true);
    expect_eq(hist.maxUs() == null, true);
    expect_eq(hist.meanUs() == null, true);
    expect_eq(hist.quantile(0.5) == null, true);

    hist.observe(100);
    expect_eq(hist.quantile(-0.1) == null, true);
    expect_eq(hist.quantile(1.1) == null, true);
    expect_eq(hist.quantile(nan_f64) == null, true);

    // Empty metric_name / tiny out_buf -> null
    let ok_buf: [1024]u8 = [0; 1024];
    let tiny_buf: [16]u8 = [0; 16];
    expect_eq(hist.renderPrometheus("", ok_buf[0..]) == null, true);
    expect_eq(hist.renderPrometheus("lat", tiny_buf[0..]) == null, true);

    // Empty service_name / tiny out_buf for OTLP -> null
    let slots: [1]OtlpSpanRecord = [OtlpSpanRecord{}; 1];
    let batch = OtlpSpanBatch.init(slots[0..]);
    expect_eq(batch.renderOtlpJson("", ok_buf[0..]) == null, true);
    expect_eq(batch.renderOtlpJson("svc", tiny_buf[0..]) == null, true);
}

fn main() {
    test("1. obs.telemetry_export: histogram observe, stats & p50/p95/p99 quantiles", () => {
        testHistogramObserveAndQuantiles()?;
    });
    test("2. obs.telemetry_export: custom bucket bounds & validation", () => {
        testCustomBoundsValidation()?;
    });
    test("3. obs.telemetry_export: Prometheus/OpenMetrics text exposition", () => {
        testRenderPrometheusHistogram()?;
    });
    test("4. obs.telemetry_export: OTLP span batch JSON export & escaping", () => {
        testOtlpSpanBatchExportAndEscaping()?;
    });
    test("5. obs.telemetry_export: OTLP batch drops, clear reuse & empty batch", () => {
        testOtlpSpanBatchDropsClearAndEmpty()?;
    });
    test("6. obs.telemetry_export: defensive guards (empty, invalid q, tiny buf)", () => {
        testDefensiveGuards()?;
    });
}
