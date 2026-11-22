// Source: cp/cap/test/test_timeseries_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:timeseries & cap.timeseries

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TimeSeriesError,
    AggOp,
    DataPoint,
    AnomalyPoint,
    SeriesSummary,
    TimeSeries,
} from "cap:time";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

fn testPushRetentionGuardsAndBinarySearch(): !void {
    const alloc = std.heap.page_allocator;
    let ts = TimeSeries.init(alloc, "cpu.util", 5000);
    defer ts.deinit();

    // 1. Monotonic push + NaN/Inf/OutOfOrder guards
    ts.push(1000, 10.0)?;
    ts.push(2000, 20.0)?;
    ts.push(3000, 30.0)?;

    let nan_rejected = false;
    ts.push(3500, std.math.nan(f64)) catch |e| {
        if (e == TimeSeriesError.InvalidValue) nan_rejected = true;
    };
    expect_eq(nan_rejected, true);

    let inf_rejected = false;
    ts.push(3500, std.math.inf(f64)) catch |e| {
        if (e == TimeSeriesError.InvalidValue) inf_rejected = true;
    };
    expect_eq(inf_rejected, true);

    let ooo_rejected = false;
    ts.push(2500, 25.0) catch |e| {
        if (e == TimeSeriesError.OutOfOrderTimestamp) ooo_rejected = true;
    };
    expect_eq(ooo_rejected, true);

    // 2. Retention window eviction: retention_ms = 5000
    // At ts = 7000, cutoff = 2000 -> ts = 1000 evicted, 2000..7000 retained.
    ts.push(4000, 40.0)?;
    ts.push(6000, 60.0)?;
    ts.push(7000, 70.0)?;
    expect_eq(ts.len(), 5 as usize);
    expect_eq(ts.items()[0].ts_ms, 2000 as i64);

    // Push ts = 12000 -> cutoff = 7000 -> 2000, 3000, 4000, 6000 evicted; 7000 & 12000 retained.
    ts.push(12000, 120.0)?;
    expect_eq(ts.len(), 2 as usize);
    expect_eq(ts.items()[0].ts_ms, 7000 as i64);
    expect_eq(ts.items()[1].ts_ms, 12000 as i64);

    // 3. O(log N) binary-search queryRange on a larger series
    let big = TimeSeries.init(alloc, "mem.rss", 0);
    defer big.deinit();
    for (i of 0..100) {
        const t = (i as i64) * 100;
        big.push(t, (i as f64) * 1.5)?;
    }
    const sub = big.queryRange(alloc, 2500, 3000)?;
    defer alloc.free(sub);
    expect_eq(sub.len, 6 as usize); // 2500, 2600, 2700, 2800, 2900, 3000
    expect_eq(sub[0].ts_ms, 2500 as i64);
    expect_eq(sub[5].ts_ms, 3000 as i64);
    expect_eq(approxEq(sub[0].value, 37.5, 0.000001), true);

    const empty_sub = big.queryRange(alloc, 50000, 60000)?;
    defer alloc.free(empty_sub);
    expect_eq(empty_sub.len, 0 as usize);
}

fn testDownsampleAndSummarize(): !void {
    const alloc = std.heap.page_allocator;
    let ts = TimeSeries.init(alloc, "req.latency", 0);
    defer ts.deinit();

    // Bucket 0 [1000, 2000): 1000->10, 1200->20, 1500->30, 1800->40, 1900->100
    ts.push(1000, 10.0)?;
    ts.push(1200, 20.0)?;
    ts.push(1500, 30.0)?;
    ts.push(1800, 40.0)?;
    ts.push(1900, 100.0)?;
    // Bucket 1 [2000, 3000): 2100->5, 2900->15
    ts.push(2100, 5.0)?;
    ts.push(2900, 15.0)?;

    const avg_pts = ts.downsample(alloc, 1000, 2999, 1000, AggOp.Avg)?;
    defer alloc.free(avg_pts);
    expect_eq(avg_pts.len, 2 as usize);
    expect_eq(avg_pts[0].ts_ms, 1000 as i64);
    expect_eq(approxEq(avg_pts[0].value, 40.0, 0.000001), true);
    expect_eq(avg_pts[1].ts_ms, 2000 as i64);
    expect_eq(approxEq(avg_pts[1].value, 10.0, 0.000001), true);

    const max_pts = ts.downsample(alloc, 1000, 2999, 1000, AggOp.Max)?;
    defer alloc.free(max_pts);
    expect_eq(approxEq(max_pts[0].value, 100.0, 0.000001), true);
    expect_eq(approxEq(max_pts[1].value, 15.0, 0.000001), true);

    const min_pts = ts.downsample(alloc, 1000, 2999, 1000, AggOp.Min)?;
    defer alloc.free(min_pts);
    expect_eq(approxEq(min_pts[0].value, 10.0, 0.000001), true);
    expect_eq(approxEq(min_pts[1].value, 5.0, 0.000001), true);

    const sum_pts = ts.downsample(alloc, 1000, 2999, 1000, AggOp.Sum)?;
    defer alloc.free(sum_pts);
    expect_eq(approxEq(sum_pts[0].value, 200.0, 0.000001), true);
    expect_eq(approxEq(sum_pts[1].value, 20.0, 0.000001), true);

    const cnt_pts = ts.downsample(alloc, 1000, 2999, 1000, AggOp.Count)?;
    defer alloc.free(cnt_pts);
    expect_eq(approxEq(cnt_pts[0].value, 5.0, 0.000001), true);
    expect_eq(approxEq(cnt_pts[1].value, 2.0, 0.000001), true);

    // P95 on [10, 20, 30, 40, 100]: pos = 0.95 * 4 = 3.8 -> 40 + 0.8 * (100 - 40) = 88.0
    const p95_pts = ts.downsample(alloc, 1000, 2999, 1000, AggOp.P95)?;
    defer alloc.free(p95_pts);
    expect_eq(approxEq(p95_pts[0].value, 88.0, 0.000001), true);

    // Summarize: values = [10, 20, 30, 40, 100, 5, 15], sum = 220, count = 7
    const s: SeriesSummary = ts.summarize()?;
    expect_eq(s.count, 7 as usize);
    expect_eq(approxEq(s.min, 5.0, 0.000001), true);
    expect_eq(approxEq(s.max, 100.0, 0.000001), true);
    expect_eq(approxEq(s.sum, 220.0, 0.000001), true);
    expect_eq(approxEq(s.mean, 220.0 / 7.0, 0.000001), true);
    expect_eq(approxEq(s.first, 10.0, 0.000001), true);
    expect_eq(approxEq(s.last, 15.0, 0.000001), true);
    expect_eq(s.stddev > 0.0, true);
}

fn testRateWithCounterReset(): !void {
    const alloc = std.heap.page_allocator;
    let ts = TimeSeries.init(alloc, "http.requests_total", 0);
    defer ts.deinit();

    ts.push(1000, 100.0)?;
    ts.push(3000, 160.0)?; // +60 in 2.0s => 30.0 / sec
    ts.push(5000, 40.0)?;  // Counter reset (40 < 160)! New cycle dv = 40.0 in 2.0s => 20.0 / sec
    ts.push(7000, 120.0)?; // +80 in 2.0s => 40.0 / sec

    const rates = ts.rate(alloc, 1000, 7000)?;
    defer alloc.free(rates);
    expect_eq(rates.len, 3 as usize);
    expect_eq(rates[0].ts_ms, 3000 as i64);
    expect_eq(approxEq(rates[0].value, 30.0, 0.000001), true);
    expect_eq(rates[1].ts_ms, 5000 as i64);
    expect_eq(approxEq(rates[1].value, 20.0, 0.000001), true);
    expect_eq(rates[2].ts_ms, 7000 as i64);
    expect_eq(approxEq(rates[2].value, 40.0, 0.000001), true);
}

fn testSmaEmaAndZScoreAnomalies(): !void {
    const alloc = std.heap.page_allocator;
    let ts = TimeSeries.init(alloc, "agent.tokens_per_sec", 0);
    defer ts.deinit();

    // Baseline ~10.0 with a single injected spike at ts = 6000 (value = 100.0)
    ts.push(1000, 10.0)?;
    ts.push(2000, 12.0)?;
    ts.push(3000, 11.0)?;
    ts.push(4000, 9.0)?;
    ts.push(5000, 10.0)?;
    ts.push(6000, 100.0)?; // Spike!
    ts.push(7000, 10.0)?;
    ts.push(8000, 11.0)?;

    const sma_pts = ts.sma(alloc, 3)?;
    defer alloc.free(sma_pts);
    expect_eq(sma_pts.len, 8 as usize);
    expect_eq(approxEq(sma_pts[0].value, 10.0, 0.000001), true);
    expect_eq(approxEq(sma_pts[1].value, 11.0, 0.000001), true); // (10+12)/2
    expect_eq(approxEq(sma_pts[2].value, 11.0, 0.000001), true); // (10+12+11)/3
    expect_eq(approxEq(sma_pts[5].value, 39.6666666, 0.0001), true); // (9+10+100)/3

    const ema_pts = ts.ema(alloc, 0.5)?;
    defer alloc.free(ema_pts);
    expect_eq(ema_pts.len, 8 as usize);
    expect_eq(approxEq(ema_pts[0].value, 10.0, 0.000001), true);
    expect_eq(approxEq(ema_pts[1].value, 11.0, 0.000001), true); // 0.5*12 + 0.5*10

    const anomalies = ts.detectAnomaliesZScore(alloc, 2.0)?;
    defer alloc.free(anomalies);
    expect_eq(anomalies.len, 1 as usize);
    expect_eq(anomalies[0].ts_ms, 6000 as i64);
    expect_eq(approxEq(anomalies[0].value, 100.0, 0.000001), true);
    expect_eq(anomalies[0].z_score >= 2.0, true);
}

fn testCapTimeseriesDynamicMountAndJson(): !void {
    const alloc = std.heap.page_allocator;
    let ts = cap.time.TimeSeries.init(alloc, "mesh.rtt", 0);
    defer ts.deinit();

    ts.push(100, 1.5)?;
    ts.push(200, 2.5)?;

    const pts = ts.downsample(alloc, 100, 200, 100, cap.time.AggOp.Last)?;
    defer alloc.free(pts);
    expect_eq(pts.len, 2 as usize);
    expect_eq(approxEq(pts[1].value, 2.5, 0.000001), true);

    const json = ts.toJson()?;
    defer alloc.free(json);
    expect_eq(std.mem.indexOf(u8, json, "\"name\":\"mesh.rtt\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"count\":2") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"ts_ms\":100") != null, true);
}

fn main() {
    test("1. push, retention_ms eviction, NaN/Inf/OutOfOrder guards & O(log N) queryRange", () => {
        testPushRetentionGuardsAndBinarySearch()?;
    });
    test("2. downsample aligned buckets (Avg/Max/Min/Sum/Count/P95) & summarize", () => {
        testDownsampleAndSummarize()?;
    });
    test("3. rate monotonic counter per-second velocity & counter reset handling", () => {
        testRateWithCounterReset()?;
    });
    test("4. sma/ema smoothing & detectAnomaliesZScore spike detection", () => {
        testSmaEmaAndZScoreAnomalies()?;
    });
    test("5. import cap from \"cap\"; cap.timeseries dynamic lazy mount & toJson", () => {
        testCapTimeseriesDynamicMountAndJson()?;
    });
}
