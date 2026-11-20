// Source: cp/cap/test/test_histogram_production_deep.cp
// Deep Production Verification Suite for cap:histogram & cap.histogram
// Covers:
//   1. Log-linear sub-bucketing accuracy & P50/P95/P99/P99.9/P99.99 percentiles across us-to-hour range
//   2. Coordinated Omission correction & lossless multi-core merge (plus IncompatibleHistogram check)
//   3. Out-of-range & invalid precision/range/window error handling
//   4. EWMA (1m/5m/15m & custom alpha) & SlidingWindowCounter time progression
//   5. `import cap from "cap"; cap.histogram` dynamic mounting, Prometheus/Summary export & zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HdrHistogram,
    HistogramError,
    Ewma,
    SlidingWindowCounter,
    formatSummary,
    formatPrometheusBuckets,
} from "cap:histogram";

test("cap:histogram - log-linear bucketing accuracy and P50/P95/P99/P99.9/P99.99") {
    const alloc = std.testing.allocator;

    // 1 us to 1 hour (3,600,000,000 us) with 3 significant digits (~0.1% accuracy)
    let hist = HdrHistogram.init(alloc, 1, 3_600_000_000, 3) catch unreachable;
    defer hist.deinit();

    // Record 10,000 samples from 1 to 10,000 us
    let v: u64 = 1;
    while (v <= 10_000) : (v += 1) {
        hist.recordValue(v) catch unreachable;
    }

    // Also record one tail spike at 1,000,000 us (1 second)
    hist.recordValue(1_000_000) catch unreachable;

    expect_eq(hist.totalCount(), 10_001);
    expect_eq(hist.minValue(), 1);
    expect_eq(hist.maxValue(), 1_000_000);

    const p50 = hist.valueAtPercentile(50.0);
    const p95 = hist.valueAtPercentile(95.0);
    const p99 = hist.valueAtPercentile(99.0);
    const p999 = hist.valueAtPercentile(99.9);
    const p9999 = hist.valueAtPercentile(99.99);

    // Within 0.2% tolerance for 3 significant digits
    expect_eq(p50 >= 5000 && p50 <= 5020, true);
    expect_eq(p95 >= 9500 && p95 <= 9530, true);
    expect_eq(p99 >= 9900 && p99 <= 9930, true);
    expect_eq(p999 >= 9990 && p999 <= 10020, true);
    expect_eq(p9999 >= 9999 && p9999 <= 1_001_000, true);

    const pct_le_5000 = hist.percentileAtOrBelow(5000);
    expect_eq(pct_le_5000 > 49.5 && pct_le_5000 < 50.5, true);

    expect_eq(hist.mean() > 5000.0, true);
    expect_eq(hist.stddev() > 2800.0, true);

    // Verify iterRecorded() traverses non-empty buckets monotonically
    let it = hist.iterRecorded();
    let iter_total: u64 = 0;
    let prev_to: u64 = 0;
    while (b := it.next()) {
        expect_eq(b.count > 0, true);
        expect_eq(b.value_from <= b.value_to, true);
        expect_eq(b.value_to >= prev_to, true);
        prev_to = b.value_to;
        iter_total = b.cumulative_count;
    }
    expect_eq(iter_total, 10_001);

    hist.reset();
    expect_eq(hist.totalCount(), 0);
    expect_eq(hist.minValue(), 0);
    expect_eq(hist.maxValue(), 0);
}

test("cap:histogram - Coordinated Omission correction and lossless multi-core merge") {
    const alloc = std.testing.allocator;

    let h1 = HdrHistogram.init(alloc, 1, 10_000_000, 3) catch unreachable;
    defer h1.deinit();
    let h2 = HdrHistogram.init(alloc, 1, 10_000_000, 3) catch unreachable;
    defer h2.deinit();

    // Normal fast samples on shard 1: 100 samples of 100 us
    h1.recordValues(100, 100) catch unreachable;

    // Stall of 50,000 us with expected interval 10,000 us:
    // Should record 50,000 plus synthetic 40,000, 30,000, 20,000, 10,000 (5 total samples)
    h1.recordValueWithExpectedInterval(50_000, 10_000) catch unreachable;
    expect_eq(h1.totalCount(), 105);

    // Shard 2 records 50 samples of 200 us and 5 samples of 20,000 us
    h2.recordValues(200, 50) catch unreachable;
    h2.recordValues(20_000, 5) catch unreachable;

    // Lossless merge
    h1.merge(h2) catch unreachable;
    expect_eq(h1.totalCount(), 160);
    expect_eq(h1.minValue(), 100);
    expect_eq(h1.maxValue(), 50_000);
    expect_eq(h1.valueAtPercentile(95.0) >= 20_000, true);

    // Incompatible histogram merge error
    let diff_prec = HdrHistogram.init(alloc, 1, 10_000_000, 2) catch unreachable;
    defer diff_prec.deinit();
    let incompat_err = false;
    h1.merge(diff_prec) catch |e| {
        if (e == HistogramError.IncompatibleHistogram) {
            incompat_err = true;
        }
    };
    expect_eq(incompat_err, true);
}

test("cap:histogram - invalid precision, range, window and ValueOutOfRange errors") {
    const alloc = std.testing.allocator;

    // InvalidPrecision: 0 and 6
    let err_p0 = false;
    * = HdrHistogram.init(alloc, 1, 1000, 0) catch |e| {
        if (e == HistogramError.InvalidPrecision) {
            err_p0 = true;
        }
    };
    expect_eq(err_p0, true);

    let err_p6 = false;
    * = HdrHistogram.init(alloc, 1, 1000, 6) catch |e| {
        if (e == HistogramError.InvalidPrecision) {
            err_p6 = true;
        }
    };
    expect_eq(err_p6, true);

    // InvalidRange: lowest=0 or highest < 2*lowest
    let err_r0 = false;
    * = HdrHistogram.init(alloc, 0, 1000, 2) catch |e| {
        if (e == HistogramError.InvalidRange) {
            err_r0 = true;
        }
    };
    expect_eq(err_r0, true);

    let err_r_small = false;
    * = HdrHistogram.init(alloc, 100, 150, 2) catch |e| {
        if (e == HistogramError.InvalidRange) {
            err_r_small = true;
        }
    };
    expect_eq(err_r_small, true);

    // ValueOutOfRange
    let hist = HdrHistogram.init(alloc, 1, 1000, 2) catch unreachable;
    defer hist.deinit();
    let err_oor = false;
    hist.recordValue(1001) catch |e| {
        if (e == HistogramError.ValueOutOfRange) {
            err_oor = true;
        }
    };
    expect_eq(err_oor, true);

    // InvalidWindow on SlidingWindowCounter & Ewma
    let err_win = false;
    * = SlidingWindowCounter.init(alloc, 10, 20) catch |e| {
        if (e == HistogramError.InvalidWindow) {
            err_win = true;
        }
    };
    expect_eq(err_win, true);

    let err_ewma = false;
    * = Ewma.init(0.0, 5.0) catch |e| {
        if (e == HistogramError.InvalidPrecision) {
            err_ewma = true;
        }
    };
    expect_eq(err_ewma, true);
}

test("cap:histogram - EWMA and SlidingWindowCounter time progression") {
    const alloc = std.testing.allocator;

    // EWMA 1m (5s tick)
    let ewma1 = Ewma.initOneMinute();
    ewma1.update(500);
    ewma1.tick();
    // First tick initializes rate to 500 / 5 = 100.0 req/s
    expect_eq(ewma1.rate() > 99.9 && ewma1.rate() < 100.1, true);

    // Next tick with 0 events decays rate
    ewma1.tick();
    expect_eq(ewma1.rate() < 100.0 && ewma1.rate() > 90.0, true);

    // Custom EWMA with alpha = 0.5, interval = 1.0s
    let custom_ewma = Ewma.init(0.5, 1.0) catch unreachable;
    custom_ewma.update(100);
    custom_ewma.tick(); // rate = 100
    custom_ewma.update(200);
    custom_ewma.tick(); // rate = 100 + 0.5*(200-100) = 150
    expect_eq(custom_ewma.rate() > 149.9 && custom_ewma.rate() < 150.1, true);

    // 5m & 15m constructors smoke check
    let ewma5 = Ewma.initFiveMinutes();
    ewma5.update(50);
    ewma5.tick();
    expect_eq(ewma5.rate() > 9.9 && ewma5.rate() < 10.1, true);

    let ewma15 = Ewma.initFifteenMinutes();
    ewma15.update(25);
    ewma15.tick();
    expect_eq(ewma15.rate() > 4.9 && ewma15.rate() < 5.1, true);

    // SlidingWindowCounter: 10,000ms window, 10 buckets (1,000ms per bucket)
    let sw = SlidingWindowCounter.init(alloc, 10_000, 10) catch unreachable;
    defer sw.deinit();

    sw.record(1_000, 20);
    sw.record(5_500, 30);
    expect_eq(sw.sum(5_500), 50);
    expect_eq(sw.ratePerSec(5_500) > 4.9 && sw.ratePerSec(5_500) < 5.1, true);

    // Advance to 11,000ms: tick 1 (1,000ms) expires, tick 5 (5,500ms) remains
    expect_eq(sw.sum(11_000), 30);

    // Advance past entire window: all expire
    expect_eq(sw.sum(25_000), 0);

    sw.record(25_500, 10);
    expect_eq(sw.sum(25_500), 10);
    sw.reset();
    expect_eq(sw.sum(25_500), 0);
}

test("cap.histogram - dynamic mounting, Summary/Prometheus export, zero-leak") {
    const alloc = std.testing.allocator;

    let h = cap.histogram.HdrHistogram.init(alloc, 1, 100_000, 3) catch unreachable;
    defer h.deinit();

    h.recordValue(50) catch unreachable;
    h.recordValue(200) catch unreachable;
    h.recordValue(1_500) catch unreachable;

    const summary = formatSummary(alloc, h) catch unreachable;
    defer alloc.free(summary);
    expect_eq(std.mem.indexOf(u8, summary, "count=3") != null, true);
    expect_eq(std.mem.indexOf(u8, summary, "p99=") != null, true);

    const bounds: [3]u64 = [100, 500, 2000];
    const prom = formatPrometheusBuckets(alloc, h, "http_latency_us", &bounds) catch unreachable;
    defer alloc.free(prom);
    expect_eq(std.mem.indexOf(u8, prom, "http_latency_us_bucket{le=\"100\"} 1") != null, true);
    expect_eq(std.mem.indexOf(u8, prom, "http_latency_us_bucket{le=\"500\"} 2") != null, true);
    expect_eq(std.mem.indexOf(u8, prom, "http_latency_us_bucket{le=\"+Inf\"} 3") != null, true);
    expect_eq(std.mem.indexOf(u8, prom, "http_latency_us_count 3") != null, true);
}
